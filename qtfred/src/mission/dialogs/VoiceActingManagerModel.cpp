#include <ship/ship.h>
#include "missionui/missioncmdbrief.h"
#include "mission/missionbriefcommon.h"
#include "mission/missionmessage.h"
#include "mission/missiongoals.h"
#include "hud/hudtarget.h"
#include "parse/sexp.h"
#include "iff_defs/iff_defs.h"
#include "mission/missiongoals.h"
#include <math.h>
#include "mission/dialogs/VoiceActingManagerModel.h"

#include <algorithm>
#include <limits>

typedef fso::fred::Editor::VoiceActingManagerSettings::ExportOption ExportOption;

namespace {
void replaceStr(SCP_string& str, const char* oldStr, const char* newStr) {
	SCP_vector<size_t> oldStrLocations;
	size_t startPos = 0;
	size_t match = str.find(oldStr, startPos);
	while (match != SCP_string::npos) {
		oldStrLocations.emplace_back(match);
		startPos = match + strlen(oldStr);
		match = str.find(oldStr, startPos);
	}
	std::reverse(oldStrLocations.begin(), oldStrLocations.end());
	for (const size_t oldStrPos : oldStrLocations) {
		str.replace(oldStrPos, strlen(oldStr), newStr);
	}
}
const MMessage* INVALID_MESSAGE = (MMessage*)std::numeric_limits<std::size_t>::max();
} // namespace

namespace fso {
namespace fred {
namespace dialogs {

	VoiceActingManagerModel::VoiceActingManagerModel(QObject* parent, EditorViewport* viewport) :
	AbstractDialogModel(parent, viewport) {

	initializeData();
}

void VoiceActingManagerModel::initializeData() {
	_editor->exportVoiceActingManagerSettings(_settings);
	
	modelChanged();
}

bool VoiceActingManagerModel::apply() {

	_editor->importVoiceActingManagerSettings(_settings);
	// TODO build example
	return true;
}
void VoiceActingManagerModel::reject() {
	// nothing to do? TODO check
}

const SCP_string& VoiceActingManagerModel::get_suffix() const
{
	Assert(_settings._extensionIndex >= 0);
	Assert(_settings._extensionIndex < _extensionOptions.size());
	return _extensionOptions[_settings._extensionIndex];
}

int VoiceActingManagerModel::calc_digits(int size) const
{
	if (size >= 10000)
		return 5;
	else if (size >= 1000)	// I hope we never hit this!
		return 4;
	else if (size >= 100)
		return 3;
	else
		return 2;
}

void VoiceActingManagerModel::build_example()
{
	// pick a default

	if (_settings._abbrevCommandBriefing != "")
		build_example(_settings._abbrevCommandBriefing);
	else if (_settings._abbrevBriefing != "")
		build_example(_settings._abbrevBriefing);
	else if (_settings._abbrevDebriefing != "")
		build_example(_settings._abbrevDebriefing);
	else if (_settings._abbrevMessage != "")
		build_example(_settings._abbrevMessage);
	else
		build_example("");
}

void VoiceActingManagerModel::build_example(const SCP_string& section)
{
	if (section == "")
	{
		_example = "";
		return;
	}

	_example = generate_filename(section, 1, 2, INVALID_MESSAGE);
}

SCP_string VoiceActingManagerModel::generate_filename(const SCP_string& section, int number, size_t digits, const MMessage *message)
{
	if (section == "")
		return "none.wav";

	int i;
	SCP_string str = "";
	SCP_string num;

	// build prefix
	if (_settings._abbrevCampaign != "")
		str = str + _settings._abbrevCampaign;
	if (_settings._abbrevMission != "")
		str = str + _settings._abbrevMission;
	str = str + section;

	// build number
	num = std::to_string(number);
	digits -= num.length();
	for (i = 0; i < digits; i++)
	{
		num = "0" + num;
	}
	str = str + num;


	const SCP_string suffix = get_suffix();
	// append sender name if supposed to and I have been passed a message
	// to get the sender from
	if (message != NULL && _settings._useSenderInFilename) {
		size_t allow_to_copy = NAME_LENGTH - suffix.length() - str.length();
		char sender[NAME_LENGTH];
		if (message == INVALID_MESSAGE) {
			strcpy_s(sender, "Alpha 1");
		}
		else {
			get_valid_sender(sender, sizeof(sender), message);
		}

		// truncate sender to that we don't overflow filename
		sender[allow_to_copy] = '\0';
		size_t j;
		for (j = 0; sender[j] != '\0'; j++) {
			// lower case letter
			sender[j] = (char)tolower(sender[j]);

			// replace any non alpha numeric with a underscore
			if (!isalnum(sender[j]))
				sender[j] = '_';
		}

		// flatten muliple underscores
		j = 1;
		while (sender[j] != '\0') {
			if (sender[j - 1] == '_' && sender[j] == '_') {
				size_t k;
				for (k = j + 1; sender[k] != '\0'; k++)
					sender[k - 1] = sender[k];
				sender[k - 1] = '\0';
			}
			else {
				// only increment on rounds when I am not moving the string down
				j++;
			}
		}
		str = str + sender;
	}

	// suffix
	str = str + get_suffix();

	Assert(str.length() < NAME_LENGTH);

	return str;
}

void VoiceActingManagerModel::OnGenerateFileNames()
{
	int i;
	int digits;
	size_t modified_filenames = 0;

	// command briefings
	digits = calc_digits(Cmd_briefs[0].num_stages);
	for (i = 0; i < Cmd_briefs[0].num_stages; i++)
	{
		char *filename = Cmd_briefs[0].stage[i].wave_filename;

		// generate only if we're replacing or if it has a replaceable name
		if (_settings._replaceFilenames || !strlen(filename) || message_filename_is_generic(filename))
		{
			strncpy(filename, generate_filename(_settings._abbrevCommandBriefing, i + 1, digits).c_str(), sizeof(filename) - 1);
			modified_filenames++;
		}
	}

	// briefings
	digits = calc_digits(Briefings[0].num_stages);
	for (i = 0; i < Briefings[0].num_stages; i++)
	{
		char *filename = Briefings[0].stages[i].voice;

		// generate only if we're replacing or if it has a replaceable name
		if (_settings._replaceFilenames || !strlen(filename) || message_filename_is_generic(filename))
		{
			strcpy(filename, generate_filename(_settings._abbrevBriefing, i + 1, digits).c_str());
			modified_filenames++;
		}
	}

	// debriefings
	digits = calc_digits(Debriefings[0].num_stages);
	for (i = 0; i < Debriefings[0].num_stages; i++)
	{
		char *filename = Debriefings[0].stages[i].voice;

		// generate only if we're replacing or if it has a replaceable name
		if (_settings._replaceFilenames || !strlen(filename) || message_filename_is_generic(filename))
		{
			strcpy(filename, generate_filename(_settings._abbrevDebriefing, i + 1, digits).c_str());
			modified_filenames++;
		}
	}

	// messages
	digits = calc_digits(Num_messages - Num_builtin_messages);
	for (i = 0; i < Num_messages - Num_builtin_messages; i++)
	{
		char *filename = Messages[i + Num_builtin_messages].wave_info.name;
		MMessage *message = &Messages[i + Num_builtin_messages];

		// generate only if we're replacing or if it has a replaceable name
		if (_settings._replaceFilenames || !strlen(filename) || message_filename_is_generic(filename))
		{
			// free existing filename
			if (filename != NULL)
				free(filename);

			// allocate new filename
			Messages[i + Num_builtin_messages].wave_info.name = strdup(generate_filename(_settings._abbrevMessage, i + 1, digits, message).c_str());
			modified_filenames++;
		}
	}

	if (modified_filenames > 0) {
		// TODO what to do here?
		// Tell FRED that we actually modified something
		//set_modified(TRUE);
	}

	// notify user that we are done and how many filenames were changed

	fileNamesGenerated(modified_filenames);

	// TODO
	// TODO we need signals like scriptGenerated(),  fileNamesGenerated()
	//char message[128] = { '\0' };
	//snprintf(message, sizeof(message) - 1, "File name generation complete. Modified " SIZE_T_ARG " messages.", modified_filenames);
	//MessageBox(message, "Woohoo!");
}

void VoiceActingManagerModel::OnGenerateScript()
{
	char pathname[256];

	// prompt to save script
	//CFileDialog dlg(FALSE, "txt", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Text files (*.txt)|*.txt||");
	//if (dlg.DoModal() != IDOK)
	//	return;

	/*SCP_string dlgPathName = dlg.GetPathName();*/
	SCP_string dlgPathName = "FIXME_TEMP_placeholder";
	// FIXME TODO limit to 255 chars, see cfile.h CF_MAX_PATHNAME_LENGTH
	_fp = cfopen(pathname, "wt", CFILE_NORMAL);
	if (!_fp)
	{
		//MessageBox("Can't open file to save.", "Error!");
		// TODO notify user
		return;
	}

	fout("%s\n", Mission_filename);
	fout("%s\n\n", The_mission.name);

	const auto exportSelection = _settings._exportSelection;

	if (exportSelection == ExportOption::Everything || exportSelection == ExportOption::CmdBriefings)
	{
		fout("\n\nCommand Briefings\n-----------------\n\n");

		for (int i = 0; i < Cmd_briefs[0].num_stages; i++)
		{
			SCP_string entry = _settings._scriptEntryFormat;
			replaceStr(entry, "\r\n", "\n");

			cmd_brief_stage *stage = &Cmd_briefs[0].stage[i];
			replaceStr(entry, "$filename", stage->wave_filename);
			replaceStr(entry, "$message", stage->text.c_str());
			replaceStr(entry, "$persona", "<no persona specified>");
			replaceStr(entry, "$sender", "<no sender specified>");

			fout("%s\n\n\n", entry.c_str());
		}
	}

	if (exportSelection == ExportOption::Everything || exportSelection == ExportOption::Briefings)
	{
		fout("\n\nBriefings\n---------\n\n");

		for (int i = 0; i < Briefings[0].num_stages; i++)
		{
			SCP_string entry = _settings._scriptEntryFormat;
			replaceStr(entry, "\r\n", "\n");

			brief_stage *stage = &Briefings[0].stages[i];
			replaceStr(entry, "$filename", stage->voice);
			replaceStr(entry, "$message", stage->text.c_str());
			replaceStr(entry, "$persona", "<no persona specified>");
			replaceStr(entry, "$sender", "<no sender specified>");

			fout("%s\n\n\n", entry.c_str());
		}
	}

	if (exportSelection == ExportOption::Everything || exportSelection == ExportOption::Debriefings)
	{
		fout("\n\nDebriefings\n-----------\n\n");

		for (int i = 0; i < Debriefings[0].num_stages; i++)
		{
			SCP_string entry = _settings._scriptEntryFormat;
			replaceStr(entry, "\r\n", "\n");

			debrief_stage *stage = &Debriefings[0].stages[i];
			replaceStr(entry, "$filename", stage->voice);
			replaceStr(entry, "$message", stage->text.c_str());
			replaceStr(entry, "$persona", "<no persona specified>");
			replaceStr(entry, "$sender", "<no sender specified>");

			fout("%s\n\n\n", entry.c_str());
		}
	}

	if (exportSelection == ExportOption::Everything || exportSelection == ExportOption::Messages)
	{
		fout("\n\nMessages\n--------\n\n");

		if (_settings._groupMessages)
		{
			SCP_vector<int> message_indexes;
			for (int i = 0; i < Num_messages - Num_builtin_messages; i++)
				message_indexes.push_back(i + Num_builtin_messages);

			group_message_indexes(message_indexes);

			for (size_t index = 0; index < message_indexes.size(); index++)
			{
				MMessage *message = &Messages[message_indexes[index]];
				export_one_message(message);
			}
		}
		else
		{
			for (int i = 0; i < Num_messages - Num_builtin_messages; i++)
			{
				MMessage *message = &Messages[i + Num_builtin_messages];
				export_one_message(message);
			}
		}
	}

	cfclose(_fp);

	// notify
	// TODO maybe return true/false
	//MessageBox("Script generation complete.", "Woohoo!");
	scriptGenerated();
}

void VoiceActingManagerModel::export_one_message(MMessage *message)
{
	SCP_string entry = _settings._scriptEntryFormat;
	replaceStr(entry, "\r\n", "\n");

	// replace file name
	replaceStr(entry, "$filename", message->wave_info.name);

	// determine and replace persona
	replaceStr(entry, "$message", message->message);
	if (message->persona_index >= 0)
		replaceStr(entry, "$persona", Personas[message->persona_index].name);
	else
		replaceStr(entry, "$persona", "<none>");

	// determine sender
	char sender[NAME_LENGTH + 1];

	get_valid_sender(sender, sizeof(sender), message);

	// replace sender (but print #Command as Command)
	if (*sender == '#')
		replaceStr(entry, "$sender", &sender[1]);
	else
		replaceStr(entry, "$sender", sender);

	fout("%s\n\n\n", entry.c_str());
}

/** Passed sender string will have either have the senders name
or '\<none\>'*/
void VoiceActingManagerModel::get_valid_sender(char *sender, size_t sender_size, const MMessage *message) {
	Assert(sender != NULL);
	Assert(message != NULL);

	strncpy(sender, get_message_sender(message->name).c_str(), sender_size);

	// strip hash if present
	if (sender[0] == '#') {
		size_t i = 1;
		for (; sender[i] != '\0'; i++) {
			sender[i - 1] = sender[i];
		}
		sender[i - 1] = '\0';
	}

	int shipnum = ship_name_lookup(sender, 1); // The player's ship is valid for this search.

	if (shipnum >= 0)
	{
		ship *shipp = &Ships[shipnum];

		// we may have to use the callsign
		if (*Fred_callsigns[shipnum])
		{
			hud_stuff_ship_callsign(sender, shipp);
		}
		// account for hidden ship names
		else if (((Iff_info[shipp->team].flags & IFFF_WING_NAME_HIDDEN) && (shipp->wingnum != -1)) || (shipp->flags[Ship::Ship_Flags::Hide_ship_name]))
		{
			hud_stuff_ship_class(sender, shipp);
		}
		// use the regular sender text
		else
		{
			end_string_at_first_hash_symbol(sender);
		}
	}
}

// TODO figure out what to do with this
//void VoiceActingManagerModel::OnSetfocusAbbrevBriefing()
//{
//	UpdateData(TRUE);
//
//	build_example(_settings._abbrevBriefing);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnSetfocusAbbrevCampaign()
//{
//	UpdateData(TRUE);
//
//	build_example();
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnSetfocusAbbrevCommandBriefing()
//{
//	UpdateData(TRUE);
//
//	build_example(_abbrevCommandBriefing);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnSetfocusAbbrevDebriefing()
//{
//	UpdateData(TRUE);
//
//	build_example(_settings._abbrevDebriefing);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnSetfocusAbbrevMessage()
//{
//	UpdateData(TRUE);
//
//	build_example(m_abbrev_message);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnSetfocusAbbrevMission()
//{
//	UpdateData(TRUE);
//
//	build_example();
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnSetfocusSuffix()
//{
//	UpdateData(TRUE);
//
//	build_example();
//
//	UpdateData(FALSE);
//}
//
//void VoiceActingManagerModel::OnChangeAbbrevBriefing()
//{
//	UpdateData(TRUE);
//
//	build_example(_settings._abbrevBriefing);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnChangeAbbrevCampaign()
//{
//	UpdateData(TRUE);
//
//	build_example();
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnChangeAbbrevCommandBriefing()
//{
//	UpdateData(TRUE);
//
//	build_example(_abbrevCommandBriefing);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnChangeAbbrevDebriefing()
//{
//	UpdateData(TRUE);
//
//	build_example(_settings._abbrevDebriefing);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnChangeAbbrevMessage()
//{
//	UpdateData(TRUE);
//
//	build_example(m_abbrev_message);
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnChangeAbbrevMission()
//{
//	UpdateData(TRUE);
//
//	build_example();
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnChangeOtherSuffix()
//{
//	UpdateData(TRUE);
//
//	build_example();
//
//	UpdateData(FALSE);
//}

//void VoiceActingManagerModel::OnChangeNoReplace()
//{
//	UpdateData(TRUE);
//}

int VoiceActingManagerModel::fout(char *format, ...)
{
	SCP_string str;
	va_list args;

	va_start(args, format);
	vsprintf(str, format, args);
	va_end(args);

	cfputs(str.c_str(), _fp);
	return 0;
}

// Loops through all the sexps and finds the sender of the specified message.  This assumes there is only one possible
// sender of the message, which is probably nearly always true (especially for voice-acted missions).
SCP_string VoiceActingManagerModel::get_message_sender(const char *message)
{
	int i;

	for (i = 0; i < Num_sexp_nodes; i++)
	{
		if (Sexp_nodes[i].type == SEXP_NOT_USED)
			continue;

		// stuff
		int op = get_operator_const(Sexp_nodes[i].text);
		int n = CDR(i);

		// find the message sexps
		if (op == OP_SEND_MESSAGE)
		{
			// the first argument is the sender; the third is the message
			if (!strcmp(message, Sexp_nodes[CDDR(n)].text))
				return Sexp_nodes[n].text;
		}
		else if (op == OP_SEND_MESSAGE_LIST)
		{
			// check the argument list
			while (n != -1)
			{
				// as before
				if (!strcmp(message, Sexp_nodes[CDDR(n)].text))
					return Sexp_nodes[n].text;

				// iterate along the list
				n = CDDDDR(n);
			}
		}
		else if (op == OP_SEND_RANDOM_MESSAGE)
		{
			// as before, sort of
			char *sender = Sexp_nodes[n].text;

			// check the argument list
			n = CDDR(n);
			while (n != -1)
			{
				if (!strcmp(message, Sexp_nodes[n].text))
					return sender;

				// iterate along the list
				n = CDR(n);
			}
		}
		else if (op == OP_TRAINING_MSG)
		{
			// just check the message
			if (!strcmp(message, Sexp_nodes[n].text))
				return "Training Message";
		}
	}

	return "<none>";
}

void VoiceActingManagerModel::group_message_indexes(SCP_vector<int> &message_indexes)
{
#ifndef NDEBUG
	size_t initial_size = message_indexes.size();
#endif

	SCP_vector<int> temp_message_indexes = message_indexes;
	message_indexes.clear();

	// add all messages found in send-message-list or send-random-message node trees
	for (int i = 0; i < Num_mission_events; i++)
	{
		mission_event *event = &Mission_events[i];
		group_message_indexes_in_tree(event->formula, temp_message_indexes, message_indexes);
	}

	// add remaining messages
	for (size_t index = 0; index < temp_message_indexes.size(); index++)
		message_indexes.push_back(temp_message_indexes[index]);

#ifndef NDEBUG
	if (initial_size > message_indexes.size())
	{
		Warning(LOCATION, "Initial size is greater than size after sorting!");
	}
	else if (initial_size < message_indexes.size())
	{
		Warning(LOCATION, "Initial size is less than size after sorting!");
	}
#endif
}

void VoiceActingManagerModel::group_message_indexes_in_tree(int node, SCP_vector<int> &source_list, SCP_vector<int> &destination_list)
{
	int op, n;

	if (node < 0)
		return;
	if (Sexp_nodes[node].type == SEXP_NOT_USED)
		return;

	// stuff
	op = get_operator_const(Sexp_nodes[node].text);
	n = CDR(node);

	if (op == OP_SEND_MESSAGE_LIST)
	{
		// check the argument list
		while (n != -1)
		{
			// the third argument is a message
			char *message_name = Sexp_nodes[CDDR(n)].text;

			// check source messages
			for (size_t i = 0; i < source_list.size(); i++)
			{
				if (!strcmp(message_name, Messages[source_list[i]].name))
				{
					// move it from source to destination
					destination_list.push_back(source_list[i]);
					source_list.erase(source_list.begin() + i);
					break;
				}
			}

			// iterate along the list
			n = CDDDDR(n);
		}
	}
	else if (op == OP_SEND_RANDOM_MESSAGE)
	{
		// check the argument list
		n = CDDR(n);
		while (n != -1)
		{
			// each argument from this point on is a message
			char *message_name = Sexp_nodes[n].text;

			// check source messages
			for (size_t i = 0; i < source_list.size(); i++)
			{
				if (!strcmp(message_name, Messages[source_list[i]].name))
				{
					// move it from source to destination
					destination_list.push_back(source_list[i]);
					source_list.erase(source_list.begin() + i);
					break;
				}
			}

			// iterate along the list
			n = CDR(n);
		}
	}

	// iterate on first element
	group_message_indexes_in_tree(CAR(node), source_list, destination_list);

	// iterate on rest of elements
	group_message_indexes_in_tree(CDR(node), source_list, destination_list);
}


}
}
}
