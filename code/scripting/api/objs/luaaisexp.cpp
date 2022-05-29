//
//

#include "luaaisexp.h"

namespace scripting {
namespace api {

lua_ai_sexp_h::lua_ai_sexp_h(sexp::LuaAISEXP* handle) : sexp_handle(handle) {
}
lua_ai_sexp_h::lua_ai_sexp_h() : sexp_handle(nullptr) {
}
bool lua_ai_sexp_h::isValid() {
	return sexp_handle != nullptr;
}

ADE_OBJ(l_LuaAISEXP, lua_ai_sexp_h, "LuaAISEXP", "Lua AI SEXP handle");

inline int sexp_function_getset_helper(lua_State* L, void (sexp::LuaAISEXP::*setter)(const luacpp::LuaFunction&), luacpp::LuaFunction (sexp::LuaAISEXP::* getter)() const) {
	lua_ai_sexp_h lua_sexp;
	luacpp::LuaFunction action_arg;
	if (!ade_get_args(L, "o|u", l_LuaAISEXP.Get(&lua_sexp), &action_arg)) {
		return ADE_RETURN_NIL;
	}

	if (!lua_sexp.isValid()) {
		return ADE_RETURN_NIL;
	}

	if (ADE_SETTING_VAR) {
		Assertion(action_arg.isValid(), "Function reference must be valid!");

		(lua_sexp.sexp_handle->*setter)(action_arg);
	}

	auto action = (lua_sexp.sexp_handle->*getter)();
	return ade_set_args(L, "u", &action);
}

ADE_VIRTVAR(ActionEnter,
	l_LuaAISEXP,
	"function(ai_helper helper, oswpt | nil arg) => boolean action",
	"The action of this AI SEXP to be executed once when the AI receives this order. Return true if the AI goal is complete.",
	"function(ai_helper helper, oswpt | nil arg) => boolean action",
	"The action function or nil on error")
{
	return sexp_function_getset_helper(L, &sexp::LuaAISEXP::setActionEnter, &sexp::LuaAISEXP::getActionEnter);
}

ADE_VIRTVAR(ActionFrame,
	l_LuaAISEXP,
	"function(ai_helper helper, oswpt | nil arg) => boolean action",
	"The action of this AI SEXP to be executed each frame while active. Return true if the AI goal is complete.",
	"function(ai_helper helper, oswpt | nil arg) => boolean action",
	"The action function or nil on error")
{
	return sexp_function_getset_helper(L, &sexp::LuaAISEXP::setActionFrame, &sexp::LuaAISEXP::getActionFrame);
}

ADE_VIRTVAR(Achievability,
	l_LuaAISEXP,
	"function(ship ship, oswpt | nil arg) => enumeration achievable",
	"An optional function that specifies whether the AI mode is achieveable. Return LUAAI_ACHIEVABLE if it can be achieved, LUAAI_NOT_YET_ACHIEVABLE if it can be achieved later and execution should be delayed, and LUAAI_UNACHIEVABLE if the AI mode will never be achievable and should be cancelled. Assumes LUAAI_ACHIEVABLE if not specified.",
	"function(ship ship, oswpt | nil arg) => enumeration achievable",
	"The achievability function or nil on error")
{
	return sexp_function_getset_helper(L, &sexp::LuaAISEXP::setAchievable, &sexp::LuaAISEXP::getAchievable);
}

ADE_VIRTVAR(TargetRestrict,
	l_LuaAISEXP,
	"function(ship ship, oswpt | nil arg) => boolean validTarget",
	"An optional function that specifies whether a target is a valid target for a player order. Result must be true and the player order +Target Restrict: must be fulfilled for the target to be valid. Assumes true if not specified.",
	"function(ship ship, oswpt | nil arg) => boolean validTarget",
	"The target restrict function or nil on error")
{
	return sexp_function_getset_helper(L, &sexp::LuaAISEXP::setTargetRestrict, &sexp::LuaAISEXP::getTargetRestrict);
}

}
}

