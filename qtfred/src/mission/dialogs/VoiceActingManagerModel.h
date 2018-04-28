#pragma once

#include "mission/dialogs/AbstractDialogModel.h"
#include "mission/missionmessage.h"

namespace fso {
namespace fred {
namespace dialogs {

class VoiceActingManagerModel : public AbstractDialogModel {
	Q_OBJECT

public:
	VoiceActingManagerModel(QObject* parent, EditorViewport* viewport);
	~VoiceActingManagerModel() override = default;

	bool apply() override;
	void reject() override;

	const std::vector<SCP_string>& getExtensionOptions() const { return _extensionOptions; }
	const Editor::VoiceActingManagerSettings& getSettings() const { return _settings; }
	const SCP_string& getExample() const { return _example; }

	int getMaxAbbrevLength() const { return Editor::VoiceActingManagerSettings::MAX_ABBREV_LENGTH; }
	int getMaxScriptFormatLength() const { return Editor::VoiceActingManagerSettings::MAX_SCRIPT_FORMAT_LENGTH; }

	// TODO modify the setAbbrev* functions to say if modify then build example
	void setAbbrevBriefing(const SCP_string& abbrevBriefing) { modify<SCP_string>(_settings._abbrevBriefing, abbrevBriefing, true); }
	void setAbbrevCampaign(const SCP_string& abbrevCampaign) { modify<SCP_string>(_settings._abbrevCampaign, abbrevCampaign, true); }
	void setAbbrevCommandBriefing(const SCP_string& abbrevCommandBriefing) { modify<SCP_string>(_settings._abbrevCommandBriefing, abbrevCommandBriefing, true); }
	void setAbbrevDebriefing(const SCP_string& abbrevDebriefing) { modify<SCP_string>(_settings._abbrevDebriefing, abbrevDebriefing, true); }
	void setAbbrevMessage(const SCP_string& abbrevMessage) { modify<SCP_string>(_settings._abbrevMessage, abbrevMessage, true); }
	void setAbbrevMission(const SCP_string& abbrevMission) { modify<SCP_string>(_settings._abbrevMission, abbrevMission, true); }
	void setReplaceFilenames(bool replaceFilenames) { modify<bool>(_settings._replaceFilenames, replaceFilenames); }
	void setScriptEntryFormat(const SCP_string& scriptEntryFormat) {
		SCP_string resizedFormat(scriptEntryFormat);
		resizedFormat.resize(Editor::VoiceActingManagerSettings::MAX_SCRIPT_FORMAT_LENGTH);
		modify<SCP_string>(_settings._scriptEntryFormat, resizedFormat);
	}
	void setExportSelection(ExportOption exportSelection) {
		modify<ExportOption>(_settings._exportSelection, exportSelection);
	}
	void setGroupMessages(bool groupMessages) { modify<bool>(_settings._groupMessages, groupMessages); }
 private:
	void initializeData();

	template<typename T>
	inline void modify(T &a, const T &b, bool isAbbrev = false);

	void OnGenerateFileNames();
	void OnGenerateScript();
	const SCP_string& get_suffix() const;
	int calc_digits(int size) const;
	void build_example();
	void build_example(const SCP_string& section);
	SCP_string generate_filename(const SCP_string& section, int number, size_t digits, const MMessage *message = NULL);
	SCP_string get_message_sender(const char *message);
	void export_one_message(MMessage *message);
	void get_valid_sender(char *sender, size_t sender_size, const MMessage *message);
	void group_message_indexes(SCP_vector<int> &message_indexes);
	void group_message_indexes_in_tree(int node, SCP_vector<int> &source_list, SCP_vector<int> &destination_list);

	CFILE *_fp;
	int fout(char *format, ...);

	std::vector<SCP_string> _extensionOptions;
	Editor::VoiceActingManagerSettings _settings;
	SCP_string _example;
signals:
	void fileNamesGenerated(size_t numFiles);
	void scriptGenerated();
};

template<typename T>
void VoiceActingManagerModel::modify(T &a, const T &b, bool isAbbrev) {
	if (a != b) {
		a = b;
		if (isAbbrev) {
			// TODO build example
		}
		modelChanged();
	}
}

}
}
}
