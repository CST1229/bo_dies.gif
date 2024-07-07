#include <Aurie/shared.hpp>
#include <YYToolkit/Shared.hpp>

#include "bodeadgif.h"

using namespace Aurie;
using namespace YYTK;

static YYTKInterface* g_YYTKInterface = nullptr;
static AurieStatus last_status = AURIE_SUCCESS;
static PFUNC_YYGMLScript oilcutscene_function = nullptr;

RValue& OilHook(
	CInstance* Self,
	CInstance* Other,
	RValue& ReturnValue,
	int ArgumentCount,
	RValue** Arguments
) {
	static double spr_player_oilintro2 = -1.0;
	if (spr_player_oilintro2 == -1.0)
		spr_player_oilintro2 = g_YYTKInterface->CallBuiltin("asset_get_index", {"spr_player_oilintro2"}).AsReal();
	static double fmod_studio_system_set_parameter_by_name = -1.0;
	if (fmod_studio_system_set_parameter_by_name == -1.0)
		fmod_studio_system_set_parameter_by_name = g_YYTKInterface->CallBuiltin("asset_get_index", {"fmod_studio_system_set_parameter_by_name"}).AsReal();

	if (oilcutscene_function != nullptr)
		oilcutscene_function(Self, Other, ReturnValue, ArgumentCount, Arguments);

	RValue value;
	last_status = g_YYTKInterface->GetBuiltin("sprite_index", Self, NULL_INDEX, value);
	if (AurieSuccess(last_status) && value.AsReal() == spr_player_oilintro2) {
		last_status = g_YYTKInterface->GetBuiltin("image_index", Self, NULL_INDEX, value);
		if (AurieSuccess(last_status)) {
			double image_index = value.AsReal();
			if (image_index >= 15.0) {
				RValue zero = RValue(0.0);
				g_YYTKInterface->SetBuiltin("image_speed", Self, NULL_INDEX, zero);
			} else if (image_index >= 11.0) {
				// bo died. rest in bo
				// mute the music
				g_YYTKInterface->CallBuiltin(
					"script_execute",
					{
						fmod_studio_system_set_parameter_by_name,
						"masterVolume",
						0.0
					}
				);

				RValue imagespeed = RValue(1.0 / 3600.0); // 1 minute
				g_YYTKInterface->SetBuiltin("image_speed", Self, NULL_INDEX, imagespeed);

				// prevent pausing in the hackiest way possible
				g_YYTKInterface->CallBuiltin("instance_destroy",
					{
						g_YYTKInterface->CallBuiltin("asset_get_index", {"obj_pauseNew"})
					}
				);

				g_YYTKInterface->CallBuiltin("variable_global_set",
					{"option_hud", false}
				);
			}
		}
	}

	return ReturnValue;
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	UNREFERENCED_PARAMETER(Module);
	UNREFERENCED_PARAMETER(ModulePath);

	last_status = ObGetInterface(
		"YYTK_Main",
		reinterpret_cast<AurieInterfaceBase*&>(g_YYTKInterface)
	);

	if (!AurieSuccess(last_status)) return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	
	// replace spr_player_oilintro2
	char filename[] = "temp_sprite.png";
	RValue handle = g_YYTKInterface->CallBuiltin("file_bin_open", {filename, 1});
	if (handle.AsReal() < 0.0) {
		return AURIE_UNAVAILABLE;
	}
	g_YYTKInterface->CallBuiltin("file_bin_rewrite", {handle});
	size_t size = (sizeof(bo_fucking_dies_gif) / sizeof(char));
	for (size_t i = 0; i < size; ++i) {
		g_YYTKInterface->CallBuiltin("file_bin_write_byte", {handle, bo_fucking_dies_gif[i]});
	}
	g_YYTKInterface->CallBuiltin("file_bin_close", {handle});
	g_YYTKInterface->CallBuiltin("sprite_replace", {
		g_YYTKInterface->CallBuiltin("asset_get_index", {"spr_player_oilintro2"}),
		filename,
		17.0,
		false,
		false,
		61.0,
		68.0
	});
	g_YYTKInterface->CallBuiltin("file_delete", {filename});

	CScript* script_data = nullptr;
	g_YYTKInterface->GetNamedRoutinePointer(
		"gml_Script_state_player_oilcutscene",
		reinterpret_cast<PVOID*>(&script_data)
	);
	MmCreateHook(
		g_ArSelfModule,
		"bo_fucking_dies.gif oil hook",
		script_data->m_Functions->m_ScriptFunction,
		OilHook,
		reinterpret_cast<PVOID*>(&oilcutscene_function)
	);

	return AURIE_SUCCESS;
}