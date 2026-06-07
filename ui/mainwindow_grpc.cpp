#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDialogButtonBox>
#include <QtConcurrent>

#include "libbox.h"

#include "db/ConfigBuilder.hpp"
#include "db/ProfileManager.hpp"
#include "db/traffic/TrafficLooper.hpp"

// ext core

std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> CreateExtCFromExtR(const std::list<std::shared_ptr<NekoGui_fmt::ExternalBuildResult>> &extRs) {
    // plz run and start in same thread
    std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> processes;
    for (const auto &extR: extRs) {
        auto extC = std::make_shared<NekoGui_sys::ExternalProcess>();
        extC->tag = extR->tag;
        extC->setProgram(extR->program);
        extC->setArguments(extR->arguments);
        extC->setEnvironment(QProcess::systemEnvironment() + extR->env);
        processes.emplace_back(extC);
    }
    return processes;
}

void MainWindow::speedtest_current_group(int mode) {
    if (!speedtestFuture.isFinished()) {
        MessageBoxWarning(software_name, "The last speed test did not exit completely, please wait. If it persists, please restart the program.");
        return;
    }

    speedtestProfiles = get_now_selected_list();
    if (speedtestProfiles.isEmpty()) return;
    auto group = NekoGui::profileManager->CurrentGroup();
    if (group->archive) return;

    if (!mode) {
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
        if (l1.isChecked()) mode |= UrlTest;
        if (l2.isChecked()) mode |= UdpTest;
        if (l3.isChecked()) mode |= SpeedTest;
        if (l4.isChecked()) mode |= IpTest;
        //
        if (mode == 0) return;
    }

    QThreadPool::globalInstance()->setMaxThreadCount(NekoGui::dataStore->test_concurrent);
    speedtestFuture = QtConcurrent::map(speedtestProfiles, [=, this](std::shared_ptr<NekoGui::ProxyEntity> &profile) {
        std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> extCs;

        QByteArray Address = profile->bean->DisplayAddress().toUtf8();
        QByteArray Url = NekoGui::dataStore->test_latency_url.toUtf8();
        int Timeout = 3000;
        QByteArray SpeedUrl = NekoGui::dataStore->test_download_url.toUtf8();
        int SpeedTimeout = NekoGui::dataStore->test_download_timeout;
        QByteArray CoreConfig;
        if (mode != TcpPing) {
            auto c = BuildConfig(profile, true, false);
            if (!c->error.isEmpty()) {
                profile->full_test_report = c->error;
                profile->Save();
                auto profileId = profile->id;
                runOnUiThread([this, profileId] {
                    refresh_proxy(profileId);
                });
                return;
            }
            //
            if (!c->extRs.empty()) {
                extCs = CreateExtCFromExtR(c->extRs);
                for (const auto &extC: extCs) {
                    extC->start();
                    extC->waitForReadyRead();
                }
            }
            //
            CoreConfig = QJsonObject2QString(c->coreConfig, false).toUtf8();
        }

        auto boxTestResult = BoxTest(mode, Address.data(), Url.data(), Timeout, SpeedUrl.data(), SpeedTimeout, CoreConfig.data());
        QStringList testResultList = QString(boxTestResult).split("\n");
        free(boxTestResult);
        //
        bool testOK;
        QStringList full_test_result;
        if (mode == TcpPing || mode == UrlTest || mode == UdpTest) {
            auto testResult = testResultList.takeFirst();
            profile->full_test_report.clear();
            profile->latency = testResult.toInt(&testOK);
            if (profile->latency == 0) profile->latency = 1;
            if (!testOK) {
                profile->latency = -1;
                MW_show_log(tr("[%1] test error: %2").arg(profile->bean->DisplayTypeAndName(), testResult));
            }
        } else {
            if (mode & UrlTest) {
                auto testResult = testResultList.takeFirst();
                testResult.toInt(&testOK);
                if (testOK)
                    full_test_result.append("Latency: " + testResult + " ms");
                else
                    full_test_result.append("Latency: Error");
            }
            if (mode & UdpTest) {
                auto testResult = testResultList.takeFirst();
                testResult.toInt(&testOK);
                if (testOK)
                    full_test_result.append("UDPLatency: " + testResult + " ms");
                else
                    full_test_result.append("UDPLatency: Error");
            }
            if (mode & SpeedTest) {
                auto testResult = testResultList.takeFirst();
                testResult.toFloat(&testOK);
                if (testOK)
                    full_test_result.append("Speed: " + testResult + " MiB/s");
                else
                    full_test_result.append("Speed: Error");
            }
            if (mode & IpTest) {
                auto testResult = testResultList.takeFirst();
                full_test_result.append(testResult);
                auto emoji = testResultList.takeFirst();
                if (!profile->bean->name.startsWith(emoji)) {
                    profile->bean->name = emoji + (profile->bean->name.isEmpty() ? "" : " " + profile->bean->name);
                }
            }
        }

        if (!full_test_result.isEmpty()) profile->full_test_report = full_test_result.join("/"); // higher priority
        profile->Save();

        auto profileId = profile->id;
        runOnUiThread([this, profileId] {
            refresh_proxy(profileId);
        });
    });
}

void MainWindow::speedtest_current() {
    ui->label_running->setText(tr("Testing"));

    runOnNewThread([=, this] {
        auto Url = NekoGui::dataStore->test_latency_url.toUtf8();
        auto boxTestResult = BoxTest(UrlTest, nullptr, Url.data(), 3000, nullptr, -1, nullptr);
        QString testResult(boxTestResult);
        free(boxTestResult);

        runOnUiThread([=, this] {
            bool testOK;
            testResult.toInt(&testOK);
            if (testOK)
                ui->label_running->setText(tr("Test Result") + ": " + testResult + " ms");
            else {
                ui->label_running->setText(tr("Test Result") + ": " + tr("Unavailable"));
                MW_show_log(QString("UrlTest : %1").arg(testResult));
            }
            refreshTimer->start(2000);
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

    // stop current running
    if (running) {
        neko_stop();
    }

    mu_state.lock();

    auto neko_start_stage2 = [=, this] {
        MW_show_log(">>>>>>>> " + tr("Starting profile %1").arg(ent->bean->DisplayTypeAndName()));

        auto CoreConfig = QJsonObject2QString(result->coreConfig, false).toUtf8();
        auto BoxStartError = BoxStart(CoreConfig.data());
        if (BoxStartError != nullptr) {
            QString boxStartError(BoxStartError);
            free(BoxStartError);
            runOnUiThread([=, this] { MessageBoxWarning("Start failed", boxStartError); });
            MW_show_log("<<<<<<<< " + tr("Failed to start profile %1").arg(ent->bean->DisplayTypeAndName()));
            return;
        }
        //
        NekoGui_traffic::trafficLooper->proxy = result->outboundStat.get();
        NekoGui_traffic::trafficLooper->items = result->outboundStats;
        NekoGui::dataStore->ignoreConnTag = result->ignoreConnTag;
        NekoGui_traffic::trafficLooper->loop_enabled = true;

        runOnUiThread([result, this] {
            running_ext = CreateExtCFromExtR(result->extRs);
            for (const auto &extC: running_ext) extC->start();
        });

        NekoGui::dataStore->started_id = ent->id;
        running = ent;

        runOnUiThread([=, this] {
            refresh_status();
            refresh_proxy(ent->id);
        });
    };

    runOnNewThread([=, this] {
        // do start
        neko_start_stage2();
        mu_state.unlock();
    });
}

void MainWindow::neko_stop(bool crash) {
    if (!running) return;
    auto id = running->id;

    mu_state.lock();

    auto neko_stop_stage2 = [=, this] {
        MW_show_log(">>>>>>>> " + tr("Stopping profile %1").arg(running->bean->DisplayTypeAndName()));

        runOnUiThread([this] {
            running_ext.clear();
        });

        NekoGui_traffic::trafficLooper->loop_enabled = false;
        for (const auto &item: NekoGui_traffic::trafficLooper->items) {
            NekoGui::profileManager->GetProfile(item->id)->Save();
        }

        if (!crash) {
            auto BoxStopError = BoxStop();
            if (BoxStopError != nullptr) {
                QString boxStopError(BoxStopError);
                free(BoxStopError);
                runOnUiThread([=, this] { MessageBoxWarning("Stop failed", boxStopError); });
                MW_show_log("<<<<<<<< " + tr("Failed to stop profile %1").arg(running->bean->DisplayTypeAndName()));
                return;
            }
        }

        NekoGui::dataStore->started_id = -1919;
        NekoGui::dataStore->need_keep_vpn_off = false;
        running = nullptr;

        runOnUiThread([=, this] {
            refresh_status();
            refresh_proxy(id);
        });
    };

    runOnNewThread([=, this] {
        // do stop
        neko_stop_stage2();
        mu_state.unlock();
    });
}
