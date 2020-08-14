#include "w_PluginManager.hpp"

#include "components/plugins/QvPluginHost.hpp"
#include "core/settings/SettingsBackend.hpp"
#include "ui/widgets/editors/w_JsonEditor.hpp"
#include "utils/QvHelpers.hpp"

PluginManageWindow::PluginManageWindow(QWidget *parent) : QvDialog(parent)
{
    setupUi(this);
    for (auto &plugin : PluginHost->AvailablePlugins())
    {
        const auto &info = PluginHost->GetPlugin(plugin)->metadata;
        auto item = new QListWidgetItem(pluginListWidget);
        item->setCheckState(PluginHost->IsPluginEnabled(info.InternalName) ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, info.InternalName);
        item->setText(info.Name + " (" + (PluginHost->GetPlugin(info.InternalName)->isLoaded ? tr("Loaded") : tr("Not loaded")) + ")");
        pluginListWidget->addItem(item);
    }
    isLoading = false;
}

PluginManageWindow::~PluginManageWindow()
{
    DEBUG(MODULE_UI, "Plugin window destructor.")
}

void PluginManageWindow::on_pluginListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    const auto plugin = PluginHost->GetPlugin(current->data(Qt::UserRole).toString());
    auto &info = plugin->metadata;
    const auto pluginUIInterface = plugin->pluginInterface->GetGUIInterface();
    if (pluginUIInterface)
        pluginIconLabel->setPixmap(pluginUIInterface->Icon().pixmap(pluginIconLabel->size() * devicePixelRatio()));
    //
    pluginNameLabel->setText(info.Name);
    pluginAuthorLabel->setText(info.Author);
    pluginDescriptionLabel->setText(info.Description);
    pluginLibPathLabel->setText(plugin->libraryPath);
    pluginStateLabel->setText(plugin->isLoaded ? tr("Loaded") : tr("Not loaded"));
    pluginTypeLabel->setText(GetPluginTypeString(info.Components).join(NEWLINE));
    //
    if (!current)
    {
        return;
    }
    if (settingsWidget || settingsWidget.get())
    {
        pluginSettingsLayout->removeWidget(settingsWidget.get());
        settingsWidget.reset();
    }
    if (!plugin->isLoaded)
    {
        pluginUnloadLabel->setVisible(true);
        pluginUnloadLabel->setText(tr("Plugin Not Loaded"));
        return;
    }
    settingsWidget = pluginUIInterface->GetSettingsWidget();
    if (settingsWidget)
    {
        pluginUnloadLabel->setVisible(false);
        pluginSettingsLayout->addWidget(settingsWidget.get());
    }
    else
    {
        pluginUnloadLabel->setVisible(true);
        pluginUnloadLabel->setText(tr("Plugin does not have settings widget."));
    }
}

void PluginManageWindow::on_pluginListWidget_itemClicked(QListWidgetItem *item)
{
    Q_UNUSED(item)
    // on_pluginListWidget_currentItemChanged(item, nullptr);
}

void PluginManageWindow::on_pluginListWidget_itemChanged(QListWidgetItem *item)
{
    if (isLoading)
        return;
    bool isEnabled = item->checkState() == Qt::Checked;
    const auto pluginInternalName = item->data(Qt::UserRole).toString();
    PluginHost->SetIsPluginEnabled(pluginInternalName, isEnabled);
    const auto info = PluginHost->GetPlugin(pluginInternalName);
    item->setText(info->metadata.Name + " (" + (info->isLoaded ? tr("Loaded") : tr("Not loaded")) + ")");
    //
    if (!isEnabled)
    {
        QvMessageBoxInfo(this, tr("Disabling a plugin"), tr("This plugin will keep loaded until the next time Qv2ray starts."));
    }
}

void PluginManageWindow::on_pluginEditSettingsJsonBtn_clicked()
{
    if (const auto &current = pluginListWidget->currentItem(); current != nullptr)
    {
        const auto &info = PluginHost->GetPlugin(current->data(Qt::UserRole).toString());
        if (!info->isLoaded)
        {
            QvMessageBoxWarn(this, tr("Plugin not loaded"), tr("This plugin is not loaded, please enable or reload the plugin to continue."));
            return;
        }
        JsonEditor w(info->pluginInterface->GetSettngs());
        auto newConf = w.OpenEditor();
        if (w.result() == QDialog::Accepted)
        {
            info->pluginInterface->UpdateSettings(newConf);
        }
    }
}

void PluginManageWindow::on_pluginListWidget_itemSelectionChanged()
{
    auto needEnable = !pluginListWidget->selectedItems().isEmpty();
    pluginEditSettingsJsonBtn->setEnabled(needEnable);
}

void PluginManageWindow::on_openPluginFolder_clicked()
{
    QDir pluginPath(QV2RAY_CONFIG_DIR + "plugins/");
    if (!pluginPath.exists())
    {
        pluginPath.mkpath(QV2RAY_CONFIG_DIR + "plugins/");
    }
#ifdef FALL_BACK_TO_XDG_OPEN
    QProcess::execute("xdg-open", { pluginPath.absolutePath() });
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(pluginPath.absolutePath()));
#endif
}

void PluginManageWindow::on_toolButton_clicked()
{
    auto address = GlobalConfig.uiConfig.language.contains("zh") ? "https://qv2ray.net/plugins/" : "https://qv2ray.net/en/plugins/";
    QDesktopServices::openUrl(QUrl(address));
}