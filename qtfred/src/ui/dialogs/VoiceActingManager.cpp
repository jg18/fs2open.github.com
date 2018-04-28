#include "VoiceActingManager.h"
#include <mission/dialogs/VoiceActingManagerModel.h>
#include "ui_VoiceActingManager.h"
#include <ui/util/SignalBlockers.h>

namespace fso {
namespace fred {
namespace dialogs {

VoiceActingManager::VoiceActingManager(FredView* parent, EditorViewport* viewport) :
	QDialog(parent),
	ui(new Ui::VoiceActingManager()),
	_model(new VoiceActingManagerModel(this, viewport)),
	_viewport(viewport) {
    ui->setupUi(this);
	ui->exampleEdit->setReadOnly(true);

	ui->campaignAbbrevEdit->setMaxLength(Editor::VoiceActingManagerSettings::MAX_ABBREV_LENGTH);
	// TODO restrict the abbrevs to alphanumeric chars
	// maybe make a vector of QLineEdit*, push the abbrevs into it, then for each edit in the vector, set max length, alphanumeric filter, etc.
	ui->missionAbbrevEdit->setMaxLength(Editor::VoiceActingManagerSettings::MAX_ABBREV_LENGTH);
	ui->cmdBriefingAbbrevEdit->setMaxLength(Editor::VoiceActingManagerSettings::MAX_ABBREV_LENGTH);
	ui->briefingAbbrevEdit->setMaxLength(Editor::VoiceActingManagerSettings::MAX_ABBREV_LENGTH);
	ui->debriefingAbbrevEdit->setMaxLength(Editor::VoiceActingManagerSettings::MAX_ABBREV_LENGTH);
	ui->messageAbbrevEdit->setMaxLength(Editor::VoiceActingManagerSettings::MAX_ABBREV_LENGTH);

	connect(parent, &FredView::viewWindowActivated, _model.get(), &VoiceActingManagerModel::apply); // TODO right?

	connect(_model.get(), &AbstractDialogModel::modelChanged, this, &VoiceActingManager::updateUI);

	// with the export radio buttons, connect to a single function while passing in one of the ExportOption enum values
	updateUI();

	// Resize the dialog to the minimum size
	resize(QDialog::sizeHint());
}

VoiceActingManager::~VoiceActingManager() {
}

void VoiceActingManager::updateUI() {
	util::SignalBlockers blockers(this);

	const auto& settings = _model->getSettings();
	ui->campaignAbbrevEdit->setText(QString::fromStdString(settings._abbrevCampaign));
	ui->missionAbbrevEdit->setText(QString::fromStdString(settings._abbrevMission));
	ui->cmdBriefingAbbrevEdit->setText(QString::fromStdString(settings._abbrevCommandBriefing));
	ui->briefingAbbrevEdit->setText(QString::fromStdString(settings._abbrevBriefing));
	ui->debriefingAbbrevEdit->setText(QString::fromStdString(settings._abbrevDebriefing));
	ui->messageAbbrevEdit->setText(QString::fromStdString(settings._abbrevMessage));

	ui->replaceFileNamesCheckBox->setChecked(settings._replaceFilenames);
	// update extensions combo
	// determine whether the "generate" buttons are enabled/disabled
	// TODO update export group box, passing in current export selection as a param
	ui->groupSendMsgListCheckBox->setEnabled(
		settings._exportSelection == Editor::VoiceActingManagerSettings::ExportOption::Messages);
	ui->exampleEdit->setText(QString::fromStdString(_model->getExample()));
}

}
}
}
