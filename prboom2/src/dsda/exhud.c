//
// Copyright(C) 2021 by Ryan Krafnick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DSDA Extended HUD
//

#include <stdio.h>

#include "am_map.h"
#include "doomstat.h"
#include "hu_stuff.h"
#include "lprintf.h"
#include "m_file.h"
#include "r_main.h"
#include "v_video.h"
#include "w_wad.h"

#include "dsda/args.h"
#include "dsda/global.h"
#include "dsda/hud_components.h"
#include "dsda/render_stats.h"
#include "dsda/settings.h"
#include "dsda/utility.h"

#include "exhud.h"

typedef struct {
  void (*init)(int x_offset, int y_offset, int vpt_flags, int* args, int arg_count, void** data);
  void (*update)(void* data);
  void (*draw)(void* data);
  const char* name;
  const int default_vpt;
  const dboolean strict;
  const dboolean off_by_default;
  const dboolean intermission;
  const dboolean not_level;
  dboolean on;
  dboolean initialized;
  void* data;
} exhud_component_t;

typedef enum {
  exhud_ammo_text,
  exhud_armor_text,
  exhud_big_ammo,
  exhud_big_armor,
  exhud_big_armor_text,
  exhud_big_artifact,
  exhud_big_health,
  exhud_big_health_text,
  exhud_composite_time,
  exhud_health_text,
  exhud_keys,
  exhud_ready_ammo_text,
  exhud_speed_text,
  exhud_stat_totals,
  exhud_tracker,
  exhud_weapon_text,
  exhud_render_stats,
  exhud_fps,
  exhud_attempts,
  exhud_local_time,
  exhud_coordinate_display,
  exhud_line_display,
  exhud_command_display,
  exhud_event_split,
  exhud_level_splits,
  exhud_minimap,
  exhud_color_test,
  exhud_component_count,
} exhud_component_id_t;

exhud_component_t components_template[exhud_component_count] = {
  [exhud_ammo_text] = {
    dsda_InitAmmoTextHC,
    dsda_UpdateAmmoTextHC,
    dsda_DrawAmmoTextHC,
    "ammo_text",
  },
  [exhud_armor_text] = {
    dsda_InitArmorTextHC,
    dsda_UpdateArmorTextHC,
    dsda_DrawArmorTextHC,
    "armor_text",
  },
  [exhud_big_ammo] = {
    dsda_InitBigAmmoHC,
    dsda_UpdateBigAmmoHC,
    dsda_DrawBigAmmoHC,
    "big_ammo",
  },
  [exhud_big_armor] = {
    dsda_InitBigArmorHC,
    dsda_UpdateBigArmorHC,
    dsda_DrawBigArmorHC,
    "big_armor",
    VPT_NOOFFSET,
  },
  [exhud_big_armor_text] = {
    dsda_InitBigArmorTextHC,
    dsda_UpdateBigArmorTextHC,
    dsda_DrawBigArmorTextHC,
    "big_armor_text",
  },
  [exhud_big_artifact] = {
    dsda_InitBigArtifactHC,
    dsda_UpdateBigArtifactHC,
    dsda_DrawBigArtifactHC,
    "big_artifact",
  },
  [exhud_big_health] = {
    dsda_InitBigHealthHC,
    dsda_UpdateBigHealthHC,
    dsda_DrawBigHealthHC,
    "big_health",
    VPT_NOOFFSET,
  },
  [exhud_big_health_text] = {
    dsda_InitBigHealthTextHC,
    dsda_UpdateBigHealthTextHC,
    dsda_DrawBigHealthTextHC,
    "big_health_text",
  },
  [exhud_composite_time] = {
    dsda_InitCompositeTimeHC,
    dsda_UpdateCompositeTimeHC,
    dsda_DrawCompositeTimeHC,
    "composite_time",
  },
  [exhud_health_text] = {
    dsda_InitHealthTextHC,
    dsda_UpdateHealthTextHC,
    dsda_DrawHealthTextHC,
    "health_text",
  },
  [exhud_keys] = {
    dsda_InitKeysHC,
    dsda_UpdateKeysHC,
    dsda_DrawKeysHC,
    "keys",
    VPT_NOOFFSET,
  },
  [exhud_ready_ammo_text] = {
    dsda_InitReadyAmmoTextHC,
    dsda_UpdateReadyAmmoTextHC,
    dsda_DrawReadyAmmoTextHC,
    "ready_ammo_text",
  },
  [exhud_speed_text] = {
    dsda_InitSpeedTextHC,
    dsda_UpdateSpeedTextHC,
    dsda_DrawSpeedTextHC,
    "speed_text",
  },
  [exhud_stat_totals] = {
    dsda_InitStatTotalsHC,
    dsda_UpdateStatTotalsHC,
    dsda_DrawStatTotalsHC,
    "stat_totals",
  },
  [exhud_tracker] = {
    dsda_InitTrackerHC,
    dsda_UpdateTrackerHC,
    dsda_DrawTrackerHC,
    "tracker",
    .strict = true,
  },
  [exhud_weapon_text] = {
    dsda_InitWeaponTextHC,
    dsda_UpdateWeaponTextHC,
    dsda_DrawWeaponTextHC,
    "weapon_text",
  },
  [exhud_render_stats] = {
    dsda_InitRenderStatsHC,
    dsda_UpdateRenderStatsHC,
    dsda_DrawRenderStatsHC,
    "render_stats",
    .strict = true,
    .off_by_default = true,
  },
  [exhud_fps] = {
    dsda_InitFPSHC,
    dsda_UpdateFPSHC,
    dsda_DrawFPSHC,
    "fps",
    .off_by_default = true,
  },
  [exhud_attempts] = {
    dsda_InitAttemptsHC,
    dsda_UpdateAttemptsHC,
    dsda_DrawAttemptsHC,
    "attempts",
  },
  [exhud_local_time] = {
    dsda_InitLocalTimeHC,
    dsda_UpdateLocalTimeHC,
    dsda_DrawLocalTimeHC,
    "local_time",
  },
  [exhud_coordinate_display] = {
    dsda_InitCoordinateDisplayHC,
    dsda_UpdateCoordinateDisplayHC,
    dsda_DrawCoordinateDisplayHC,
    "coordinate_display",
    .strict = true,
    .off_by_default = true,
  },
  [exhud_line_display] = {
    dsda_InitLineDisplayHC,
    dsda_UpdateLineDisplayHC,
    dsda_DrawLineDisplayHC,
    "line_display",
    .strict = true,
    .off_by_default = true,
  },
  [exhud_command_display] = {
    dsda_InitCommandDisplayHC,
    dsda_UpdateCommandDisplayHC,
    dsda_DrawCommandDisplayHC,
    "command_display",
    .strict = true,
    .off_by_default = true,
    .intermission = true,
  },
  [exhud_event_split] = {
    dsda_InitEventSplitHC,
    dsda_UpdateEventSplitHC,
    dsda_DrawEventSplitHC,
    "event_split",
  },
  [exhud_level_splits] = {
    dsda_InitLevelSplitsHC,
    dsda_UpdateLevelSplitsHC,
    dsda_DrawLevelSplitsHC,
    "level_splits",
    .intermission = true,
    .not_level = true,
  },
  [exhud_minimap] = {
    dsda_InitMinimapHC,
    dsda_UpdateMinimapHC,
    dsda_DrawMinimapHC,
    "minimap",
    .off_by_default = true,
  },
  [exhud_color_test] = {
    dsda_InitColorTestHC,
    dsda_UpdateColorTestHC,
    dsda_DrawColorTestHC,
    "color_test",
  },
};

typedef struct {
  const char* name;
  dboolean status_bar;
  dboolean loaded;
  exhud_component_t components[exhud_component_count];
} dsda_hud_container_t;

typedef enum {
  hud_ex,
  hud_off,
  hud_full,
  hud_null,
} dsda_hud_variant_t;

static dsda_hud_container_t containers[] = {
  [hud_ex]   = { "ex", true },
  [hud_off]  = { "off", true },
  [hud_full] = { "full", false },
  [hud_null] = { NULL }
};

static dsda_hud_container_t* container;
static exhud_component_t* components;

int dsda_show_render_stats;

int dsda_ExHudVerticalOffset(void) {
  if (container && container->status_bar)
    return g_st_height;

  return 0;
}

static void dsda_TurnComponentOn(int id) {
  if (!components[id].initialized)
    return;

  components[id].on = true;
}

static void dsda_TurnComponentOff(int id) {
  components[id].on = false;
}

static void dsda_InitializeComponent(int id, int x, int y, int vpt, int* args, int arg_count) {
  components[id].initialized = true;
  components[id].init(x, y, vpt | components[id].default_vpt | VPT_EX_TEXT,
                 args, arg_count, &components[id].data);

  if (components[id].off_by_default)
    dsda_TurnComponentOff(id);
  else
    dsda_TurnComponentOn(id);
}

static int dsda_ParseHUDConfig(char** hud_config, int line_i) {
  int i;
  int count;
  dboolean found;
  const char* line;
  char command[64];
  char args[64];

  for (++line_i; hud_config[line_i]; ++line_i) {
    line = hud_config[line_i];

    if (line[0] == '#' || line[0] == '/' || line[0] == '!' || !line[0])
      continue;

    count = sscanf(line, "%63s %63[^\n\r]", command, args);
    if (count != 2)
      I_Error("Invalid hud definition \"%s\"", line);

    // The start of another definition
    if (!strncmp(command, "doom", sizeof(command)) ||
        !strncmp(command, "heretic", sizeof(command)) ||
        !strncmp(command, "hexen", sizeof(command)))
      break;

    found = false;

    for (i = 0; i < exhud_component_count; ++i)
      if (!strncmp(command, components[i].name, sizeof(command))) {
        int x, y;
        int vpt = 0;
        int component_args[4] = { 0 };
        char alignment[16];

        found = true;

        count = sscanf(args, "%d %d %15s %d %d %d %d", &x, &y, alignment,
                        &component_args[0], &component_args[1],
                        &component_args[2], &component_args[3]);
        if (count < 3)
          I_Error("Invalid hud component args \"%s\"", line);

        if (!strncmp(alignment, "bottom_left", sizeof(alignment)))
          vpt = VPT_ALIGN_LEFT_BOTTOM;
        else if (!strncmp(alignment, "bottom_right", sizeof(alignment)))
          vpt = VPT_ALIGN_RIGHT_BOTTOM;
        else if (!strncmp(alignment, "top_left", sizeof(alignment)))
          vpt = VPT_ALIGN_LEFT_TOP;
        else if (!strncmp(alignment, "top_right", sizeof(alignment)))
          vpt = VPT_ALIGN_RIGHT_TOP;
        else if (!strncmp(alignment, "top", sizeof(alignment)))
          vpt = VPT_ALIGN_TOP;
        else if (!strncmp(alignment, "bottom", sizeof(alignment)))
          vpt = VPT_ALIGN_BOTTOM;
        else if (!strncmp(alignment, "left", sizeof(alignment)))
          vpt = VPT_ALIGN_LEFT;
        else if (!strncmp(alignment, "right", sizeof(alignment)))
          vpt = VPT_ALIGN_RIGHT;
        else
          I_Error("Invalid hud component alignment \"%s\"", line);

        dsda_InitializeComponent(i, x, y, vpt, component_args, count - 3);
      }

    if (!found)
      I_Error("Invalid hud component \"%s\"", line);
  }

  // roll back the line that wasn't part of this config
  return line_i - 1;
}

static void dsda_ParseHUDConfigs(char** hud_config) {
  const char* line;
  int line_i;
  const char* target_format;
  char hud_variant[5];

  target_format = hexen ? "hexen %s" : heretic ? "heretic %s" : "doom %s";

  for (line_i = 0; hud_config[line_i]; ++line_i) {
    line = hud_config[line_i];

    if (sscanf(line, target_format, hud_variant)) {
      for (container = containers; container->name; container++)
        if (!strncmp(container->name, hud_variant, sizeof(hud_variant))) {
          container->loaded = true;
          components = container->components;
          memcpy(components, components_template, sizeof(components_template));

          line_i = dsda_ParseHUDConfig(hud_config, line_i);

          break;
        }
    }
  }
}

static void dsda_LoadHUDConfig(void) {
  DO_ONCE
    char* hud_config = NULL;
    char** lines;
    dsda_arg_t* arg;
    int lump;
    int length = 0;

    arg = dsda_Arg(dsda_arg_hud);
    if (arg->found)
      length = M_ReadFileToString(arg->value.v_string, &hud_config);

    lump = W_GetNumForName("DSDAHUD");

    if (lump != -1) {
      if (!hud_config)
        hud_config = W_ReadLumpToString(lump);
      else {
        hud_config = Z_Realloc(hud_config, length + W_LumpLength(lump) + 2);
        hud_config[length++] = '\n'; // in case the file didn't end in a new line
        W_ReadLump(lump, hud_config + length);
        hud_config[length + W_LumpLength(lump)] = '\0';
      }
    }

    if (hud_config) {
      lines = dsda_SplitString(hud_config, "\n\r");

      if (lines) {
        dsda_ParseHUDConfigs(lines);

        Z_Free(lines);
      }

      Z_Free(hud_config);
    }
  END_ONCE
}

static dboolean dsda_HideHUD(void) {
  return dsda_Flag(dsda_arg_nodraw) ||
         (R_FullView() && !dsda_IntConfig(dsda_config_hud_displayed));
}

static dboolean dsda_HUDActive(void) {
  return container && container->loaded;
}

static void dsda_ResetActiveHUD(void) {
  container = NULL;
  components = NULL;
}

static void dsda_UpdateActiveHUD(void) {
  container = R_FullView() ? &containers[hud_full] :
              dsda_IntConfig(dsda_config_exhud) ? &containers[hud_ex] :
              &containers[hud_off];

  if (container->loaded)
    components = container->components;
  else
    dsda_ResetActiveHUD();
}

static void dsda_RefreshHUD(void) {
  if (!dsda_HUDActive())
    return;

  if (dsda_show_render_stats)
    dsda_TurnComponentOn(exhud_render_stats);

  dsda_RefreshExHudFPS();
  dsda_RefreshExHudMinimap();
  dsda_RefreshExHudLevelSplits();
  dsda_RefreshExHudCoordinateDisplay();
  dsda_RefreshExHudCommandDisplay();

  if (in_game && gamestate == GS_LEVEL)
    dsda_UpdateExHud();
}

void dsda_InitExHud(void) {
  dsda_ResetActiveHUD();

  if (dsda_HideHUD())
    return;

  dsda_LoadHUDConfig();
  dsda_UpdateActiveHUD();
  dsda_RefreshHUD();
}

void dsda_UpdateExHud(void) {
  int i;

  if (!dsda_HUDActive())
    return;

  if (automap_on)
    return;

  for (i = 0; i < exhud_component_count; ++i)
    if (
      components[i].on &&
      !components[i].not_level &&
      (!components[i].strict || !dsda_StrictMode())
    )
      components[i].update(components[i].data);
}

void dsda_DrawExHud(void) {
  int i;

  if (!dsda_HUDActive())
    return;

  if (automap_on)
    return;

  for (i = 0; i < exhud_component_count; ++i)
    if (
      components[i].on &&
      !components[i].not_level &&
      (!components[i].strict || !dsda_StrictMode())
    )
      components[i].draw(components[i].data);
}

void dsda_DrawExIntermission(void) {
  int i;

  if (!dsda_HUDActive())
    return;

  for (i = 0; i < exhud_component_count; ++i)
    if (
      components[i].on &&
      components[i].intermission &&
      (!components[i].strict || !dsda_StrictMode())
    )
      components[i].draw(components[i].data);
}

void dsda_ToggleRenderStats(void) {
  dsda_show_render_stats = !dsda_show_render_stats;

  if (!dsda_HUDActive())
    return;

  if (components[exhud_render_stats].on && !dsda_show_render_stats)
    dsda_TurnComponentOff(exhud_render_stats);
  else if (!components[exhud_render_stats].on && dsda_show_render_stats) {
    dsda_BeginRenderStats();
    dsda_TurnComponentOn(exhud_render_stats);
  }
}

void dsda_RefreshExHudFPS(void) {
  if (!dsda_HUDActive())
    return;

  if (dsda_ShowFPS())
    dsda_TurnComponentOn(exhud_fps);
  else
    dsda_TurnComponentOff(exhud_fps);
}

void dsda_RefreshExHudMinimap(void) {
  if (!dsda_HUDActive())
    return;

  if (dsda_ShowMinimap()) {
    dsda_TurnComponentOn(exhud_minimap);
    if (in_game && gamestate == GS_LEVEL)
      AM_Start(false);
  }
  else
    dsda_TurnComponentOff(exhud_minimap);
}

void dsda_RefreshExHudLevelSplits(void) {
  if (!dsda_HUDActive())
    return;

  if (dsda_ShowLevelSplits())
    dsda_TurnComponentOn(exhud_level_splits);
  else
    dsda_TurnComponentOff(exhud_level_splits);
}

void dsda_RefreshExHudCoordinateDisplay(void) {
  if (!dsda_HUDActive())
    return;

  if (dsda_CoordinateDisplay()) {
    dsda_TurnComponentOn(exhud_coordinate_display);
    dsda_TurnComponentOn(exhud_line_display);
  }
  else {
    dsda_TurnComponentOff(exhud_coordinate_display);
    dsda_TurnComponentOff(exhud_line_display);
  }
}

void dsda_RefreshExHudCommandDisplay(void) {
  if (!dsda_HUDActive())
    return;

  if (dsda_CommandDisplay())
    dsda_TurnComponentOn(exhud_command_display);
  else
    dsda_TurnComponentOff(exhud_command_display);
}
