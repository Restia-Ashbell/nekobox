#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include "libbox.h"

#include "db/Database.hpp"
#include "db/ConfigBuilder.hpp"
#include "db/traffic/TrafficLooper.hpp"
#include "sys/ExternalProcess.hpp"
#include "ui/widget/MessageBoxTimer.h"

#include <QTimer>
#include <QThread>
#include <QInputDialog>
#include <QPushButton>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QtConcurrent>

// ext core

std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> CreateExtCFromExtR(const std::list<std::shared_ptr<NekoGui_fmt::ExternalBuildResult>> &extRs, bool start) {
    // plz run and start in same thread
    std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> l;
    for (const auto &extR: extRs) {
        std::shared_ptr<NekoGui_sys::ExternalProcess> extC(new NekoGui_sys::ExternalProcess());
        extC->tag = extR->tag;
        extC->program = extR->program;
        extC->arguments = extR->arguments;
        extC->env = extR->env;
        l.emplace_back(extC);
        //
        if (start) extC->Start();
    }
    return l;
}

void MainWindow::speedtest_current_group(int mode) {
    if (!speedtestFuture.isFinished()) {
        MessageBoxWarning(software_name, "The last speed test did not exit completely, please wait. If it persists, please restart the program.");
        return;
    }

    speedtestProfiles = get_selected_or_group();
    if (speedtestProfiles.isEmpty()) return;
    auto group = NekoGui::profileManager->CurrentGroup();
    if (group->archive) return;

    int full_test_flags = 0;
    if (mode == 0) {
        full_test_flags |= TcpPing;
    } else if (mode == 1) {
        full_test_flags |= UrlTest;
    } else if (mode == 999) {
        QDialog dialog(this);
        QVBoxLayout layout(&dialog);
        dialog.setWindowTitle(tr("Test Options"));
        //
        QCheckBox l1(tr("Latency"));
        QCheckBox l2(tr("UDP latency"));
        QCheckBox l3(tr("Download speed"));
        QCheckBox l4(tr("In and Out IP"));
        //
        QDialogButtonBox box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
        connect(&box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        //
        layout.addWidget(&l1);
        layout.addWidget(&l2);
        layout.addWidget(&l3);
        layout.addWidget(&l4);
        layout.addWidget(&box);
        if (dialog.exec() != QDialog::Accepted) return;
        //
        if (l1.isChecked()) full_test_flags |= UrlTest;
        if (l2.isChecked()) full_test_flags |= UdpTest;
        if (l3.isChecked()) full_test_flags |= SpeedTest;
        if (l4.isChecked()) full_test_flags |= IpTest;
        //
        if (full_test_flags == 0) return;
    }

    QThreadPool::globalInstance()->setMaxThreadCount(NekoGui::dataStore->test_concurrent);
    speedtestFuture = QtConcurrent::map(speedtestProfiles, [=, this](std::shared_ptr<NekoGui::ProxyEntity> &profile) {
        std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> extCs;
        QSemaphore extSem;

        QByteArray Address = profile->bean->DisplayAddress().toUtf8();
        QByteArray Url = NekoGui::dataStore->test_latency_url.toUtf8();
        int Timeout = 3000;
        QByteArray SpeedUrl = NekoGui::dataStore->test_download_url.toUtf8();
        int SpeedTimeout = NekoGui::dataStore->test_download_timeout;
        QByteArray CoreConfig;
        if (full_test_flags != TcpPing) {
            auto c = BuildConfig(profile, true, false);
            if (!c->error.isEmpty()) {
                profile->full_test_report = c->error;
                profile->Save();
                auto profileId = profile->id;
                runOnUiThread([this, profileId] {
                    refresh_proxy_list(profileId);
                });
                return;
            }
            //
            if (!c->extRs.empty()) {
                runOnUiThread(
                    [&] {
                        extCs = CreateExtCFromExtR(c->extRs, true);
                        QThread::msleep(500);
                        extSem.release();
                    },
                    DS_cores);
                extSem.acquire();
            }
            //
            CoreConfig = QJsonObject2QString(c->coreConfig, false).toUtf8();
        }

        auto boxTestResult = BoxTest(full_test_flags, Address.data(), Url.data(), Timeout, SpeedUrl.data(), SpeedTimeout, CoreConfig.data());
        QStringList testResultList = QString::fromUtf8(boxTestResult).split("\n");
        free(boxTestResult);
        //
        if (!extCs.empty()) {
            runOnUiThread(
                [&] {
                    for (const auto &extC: extCs) {
                        extC->Kill();
                    }
                    extSem.release();
                },
                DS_cores);
            extSem.acquire();
        }
        //
        bool testOK;
        QStringList full_test_result;
        if (full_test_flags == TcpPing || full_test_flags == UrlTest || full_test_flags == UdpTest) {
            auto testResult = testResultList.takeFirst();
            profile->full_test_report.clear();
            profile->latency = testResult.toInt(&testOK);
            if (profile->latency == 0) profile->latency = 1;
            if (!testOK) {
                profile->latency = -1;
                MW_show_log(tr("[%1] test error: %2").arg(profile->bean->DisplayTypeAndName(), testResult));
            }
        } else {
            if (full_test_flags & UrlTest) {
                auto testResult = testResultList.takeFirst();
                testResult.toInt(&testOK);
                if (testOK)
                    full_test_result.append("Latency: " + testResult + " ms");
                else
                    full_test_result.append("Latency: Error");
            }
            if (full_test_flags & UdpTest) {
                auto testResult = testResultList.takeFirst();
                testResult.toInt(&testOK);
                if (testOK)
                    full_test_result.append("UDPLatency: " + testResult + " ms");
                else
                    full_test_result.append("UDPLatency: Error");
            }
            if (full_test_flags & SpeedTest) {
                auto testResult = testResultList.takeFirst();
                testResult.toFloat(&testOK);
                if (testOK)
                    full_test_result.append("Speed: " + testResult + " MiB/s");
                else
                    full_test_result.append("Speed: Error");
            }
            if (full_test_flags & IpTest) {
                auto testResult = testResultList.takeFirst();
                full_test_result.append(testResult);
            }
        }

        if (!full_test_result.isEmpty()) profile->full_test_report = full_test_result.join("/"); // higher priority
        profile->Save();

        auto profileId = profile->id;
        runOnUiThread([this, profileId] {
            refresh_proxy_list(profileId);
        });
    });
}

void MainWindow::speedtest_current() {
    last_test_time = QTime::currentTime();
    ui->label_running->setText(tr("Testing"));

    runOnNewThread([=, this] {
        auto Url = NekoGui::dataStore->test_latency_url.toUtf8();
        auto boxTestResult = BoxTest(UrlTest, nullptr, Url.data(), 3000, nullptr, -1, nullptr);
        QString testResult(boxTestResult);
        free(boxTestResult);
        last_test_time = QTime::currentTime();

        runOnUiThread([=, this] {
            bool testOK;
            testResult.toInt(&testOK);
            if (testOK)
                ui->label_running->setText(tr("Test Result") + ": " + testResult + " ms");
            else {
                ui->label_running->setText(tr("Test Result") + ": " + tr("Unavailable"));
                MW_show_log(QString("UrlTest : %1").arg(testResult));
            }
        });
    });
}

void MainWindow::neko_start(int _id) {
    if (NekoGui::dataStore->prepare_exit) return;

    auto ents = get_now_selected_list();
    auto ent = (_id < 0 && !ents.isEmpty()) ? ents.first() : NekoGui::profileManager->GetProfile(_id);
    if (ent == nullptr) return;

    if (select_mode) {
        emit profile_selected(ent->id);
        select_mode = false;
        refresh_status();
        return;
    }

    auto group = NekoGui::profileManager->GetGroup(ent->gid);
    if (group == nullptr || group->archive) return;

    auto result = BuildConfig(ent, false, false);
    if (!result->error.isEmpty()) {
        MessageBoxWarning("BuildConfig return error", result->error);
        return;
    }

    auto neko_start_stage2 = [=, this] {
        auto CoreConfig = QJsonObject2QString(result->coreConfig, false).toUtf8();
        auto BoxStartError = BoxStart(CoreConfig.data());
        if (BoxStartError != nullptr) {
            QString boxStartError(BoxStartError);
            free(BoxStartError);
            runOnUiThread([=, this] { MessageBoxWarning("LoadConfig return error", boxStartError); });
            return false;
        }
        //
        NekoGui_traffic::trafficLooper->proxy = result->outboundStat.get();
        NekoGui_traffic::trafficLooper->items = result->outboundStats;
        NekoGui::dataStore->ignoreConnTag = result->ignoreConnTag;
        NekoGui_traffic::trafficLooper->loop_enabled = true;

        runOnUiThread(
            [=, this] {
                auto extCs = CreateExtCFromExtR(result->extRs, true);
                NekoGui_sys::running_ext.splice(NekoGui_sys::running_ext.end(), extCs);
            },
            DS_cores);

        NekoGui::dataStore->UpdateStartedId(ent->id);
        running = ent;

        runOnUiThread([=, this] {
            refresh_status();
            refresh_proxy_list(ent->id);
        });

        return true;
    };

    if (!mu_starting.tryLock()) {
        MessageBoxWarning(software_name, "Another profile is starting...");
        return;
    }
    if (!mu_stopping.tryLock()) {
        MessageBoxWarning(software_name, "Another profile is stopping...");
        mu_starting.unlock();
        return;
    }
    mu_stopping.unlock();

    // timeout message
    auto restartMsgbox = new QMessageBox(QMessageBox::Question, software_name, tr("If there is no response for a long time, it is recommended to restart the software."),
                                         QMessageBox::Yes | QMessageBox::No, this);
    connect(restartMsgbox, &QMessageBox::accepted, this, [=, this] { MW_dialog_message("", "RestartProgram"); });
    auto restartMsgboxTimer = new MessageBoxTimer(this, restartMsgbox, 5000);

    runOnNewThread([=, this] {
        // stop current running
        if (NekoGui::dataStore->started_id >= 0) {
            runOnUiThread([=, this] { neko_stop(false, true); });
            sem_stopped.acquire();
        }
        // do start
        MW_show_log(">>>>>>>> " + tr("Starting profile %1").arg(ent->bean->DisplayTypeAndName()));
        if (!neko_start_stage2()) {
            MW_show_log("<<<<<<<< " + tr("Failed to start profile %1").arg(ent->bean->DisplayTypeAndName()));
        }
        mu_starting.unlock();
        // cancel timeout
        runOnUiThread([=, this] {
            restartMsgboxTimer->cancel();
            restartMsgboxTimer->deleteLater();
            restartMsgbox->deleteLater();
#ifdef Q_OS_LINUX
            // Check systemd-resolved
            if (NekoGui::dataStore->spmode_vpn && NekoGui::dataStore->routing->direct_dns.startsWith("local") && ReadFileText("/etc/resolv.conf").contains("systemd-resolved")) {
                MW_show_log("[Warning] The default Direct DNS may not works with systemd-resolved, you may consider change your DNS settings.");
            }
#endif
        });
    });
}

void MainWindow::neko_stop(bool crash, bool sem) {
    auto id = NekoGui::dataStore->started_id;
    if (id < 0) {
        if (sem) sem_stopped.release();
        return;
    }

    auto neko_stop_stage2 = [=, this] {
        runOnUiThread(
            [=, this] {
                while (!NekoGui_sys::running_ext.empty()) {
                    auto extC = NekoGui_sys::running_ext.front();
                    extC->Kill();
                    NekoGui_sys::running_ext.pop_front();
                }
            },
            DS_cores);

        NekoGui_traffic::trafficLooper->loop_enabled = false;
        NekoGui_traffic::trafficLooper->loop_mutex.lock();
        if (NekoGui::dataStore->traffic_loop_interval > 0) {
            NekoGui_traffic::trafficLooper->UpdateAll();
            for (const auto &item: NekoGui_traffic::trafficLooper->items) {
                NekoGui::profileManager->GetProfile(item->id)->Save();
                runOnUiThread([=, this] { refresh_proxy_list(item->id); });
            }
        }
        NekoGui_traffic::trafficLooper->loop_mutex.unlock();

        if (!crash) {
            BoxStop();
        }

        NekoGui::dataStore->UpdateStartedId(-1919);
        NekoGui::dataStore->need_keep_vpn_off = false;
        running = nullptr;

        runOnUiThread([=, this] {
            refresh_status();
            refresh_proxy_list(id);
        });

        return true;
    };

    if (!mu_stopping.tryLock()) {
        if (sem) sem_stopped.release();
        return;
    }

    // timeout message
    auto restartMsgbox = new QMessageBox(QMessageBox::Question, software_name, tr("If there is no response for a long time, it is recommended to restart the software."),
                                         QMessageBox::Yes | QMessageBox::No, this);
    connect(restartMsgbox, &QMessageBox::accepted, this, [=, this] { MW_dialog_message("", "RestartProgram"); });
    auto restartMsgboxTimer = new MessageBoxTimer(this, restartMsgbox, 5000);

    runOnNewThread([=, this] {
        // do stop
        MW_show_log(">>>>>>>> " + tr("Stopping profile %1").arg(running->bean->DisplayTypeAndName()));
        if (!neko_stop_stage2()) {
            MW_show_log("<<<<<<<< " + tr("Failed to stop, please restart the program."));
        }
        mu_stopping.unlock();
        if (sem) sem_stopped.release();
        // cancel timeout
        runOnUiThread([=, this] {
            restartMsgboxTimer->cancel();
            restartMsgboxTimer->deleteLater();
            restartMsgbox->deleteLater();
        });
    });
}

void MainWindow::CheckUpdate() {
}
