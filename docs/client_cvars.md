List of ETrun cvars.

# Banner

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawBannerPrint | 0,1 | 1 | Draw banners. Introduced in ETrun 2.0.0. |
| etr_bannerPrintX | 0-640 | 320 | Banner horizontal position. Introduced in ETrun 2.0.0. |
| etr_bannerPrintY | 0-480 | 20 | Banner vertical position. Introduced in ETrun 2.0.0. |

# CGaz

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawCGaz | 0,1,2,3,4 | 0 | Draw CGaz. Various types are available. Use 0 to disable. |
| etr_realCGaz2 | 0,1 | 0 | Adjust CGaz 2 for widescreen display. Introduced in ETrun 1.4.0. |
| etr_CGazX | 0-640 | 320 | CGaz horizontal position. Introduced in ETrun 2.0.0. |
| etr_CGazY | 0-640 | 240 | CGaz vertical position. Introduced in ETrun 2.0.0. |
| etr_CGaz2Color1 | *string* | Red | CGaz 2 primary color. Introduced in ETrun 2.0.0. |
| etr_CGaz2Color2 | *string* | Cyan | CGaz 2 secondary color. Introduced in ETrun 2.0.0. |

# Chat

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_chatX | 0-640 | 130 | Chat horizontal position. Introduced in ETrun 1.4.0. |
| cg_chatY | 0-480 | 478 | Chat vertical position. Introduced in ETrun 1.4.0. |
| cg_chatHeight | 0-8 | 8 | Height of the chat. Introduced in ETrun 1.4.0. |

# Checkpoints

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawCheckPoints | 0,1 | 1 | Display checkpoints times. |
| etr_checkPointsX | 0-640 | 320 | Checkpoints horizontal position. |
| etr_checkPointsY | 0-480 | 435 | Checkpoints vertical position. |
| etr_maxCheckPoints | *integer* | 5 | Maximum number of checkpoints displayed. |
| etr_autoLoadCheckpoints | 0,1 | 0 | Automatically load checkpoints from PB. Player must be logged in. |
| etr_checkPointsColor1 | *string* | White | Checkpoints neutral color. Introduced in ETrun 2.0.0. |
| etr_checkPointsColor2 | *string* | Green | Checkpoints color for faster runs. Introduced in ETrun 2.0.0. |
| etr_checkPointsColor3 | *string* | Red | Checkpoints color for slower runs. Introduced in ETrun 2.0.0. |

# Clock

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawClock | 0,1 | 1 | Display a clock. |

# Demos

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_autoDemo | 0,1 | 0 | Enable recording demos automatically. |
| etr_keepAllDemos | 0,1 | 1 | Keep all demos or only demos for PB records. |
| etr_autoDemoStopDelay | *integer* | 1500 | Set delay in ms before demo is stopped. Introduced in ETrun 2.0.0. |
| etr_autoDemoPrints | 0,1 | 1 | Print demo messages. Introduced in ETrun 2.0.0. |

# Events

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_onRunStart | *string* | "" | Command(s) to trigger when a run starts . Introduced in ETrun 1.4.0. |
| etr_onRunStop | *string* | "" | Command(s) to trigger when a run stops . Introduced in ETrun 1.4.0. |

# Info panel

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawInfoPanel | 0,1 | 1 | Display info panel. |
| etr_infoPanelXoffset | 0-640 | 0 | Info panel horizontal position offset. |
| etr_infoPanelYoffset | 0-480 | 0 | Info panel vertical position offset. |
| etr_infoPanelColor1 | *string* | White | Info panel neutral color. Introduced in ETrun 2.0.0. |
| etr_infoPanelColor2 | *string* | Red | Info panel color for slower jump speeds. Introduced in ETrun 2.0.0. |
| etr_minStartSpeed | *integer* | 0 | Minimum start speed desired. If unmet, speeds are printed (new in ETrun 2.0.0). Use 0 to disable. Introduced in ETrun 1.2.0. |

Note: `etr_minStartSpeed` is reset to 0 at every new game session.

# Logs

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_viewLog | 0,1 | 0 | Enable displaying the game console in a separate window from the main game window. |

# Noclip

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_noclipSpeed | *integer* | 1000 | Set movement speed while using `/noclip` command. |

# Pickups

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_pickupPrints | 0,1 | 1 | Print pickup messages. Introduced in ETrun 2.0.0. |

# Player model

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_hideOthers | 0,1 | 1 | Hide other player models. |
| etr_hideRange | *integer* | 128 | Distance expressed in game units where other player models should be hidden. |
| etr_hideMe | 0,1 | 0 | Hide your player model from other players. |

# Player position control

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_loadViewAngles | 0,1 | 1 | Enable restoring player view angles when loading its position. |
| etr_loadWeapon | 0,1 | 1 | Enable restoring player weapon when loading its position. |
| etr_autoLoad | 0,1 | 1 | Enable automatically restoring player position when he gets killed. |

# Popups

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_numPopups | 0-16 | 5 | Maximum number of popups to display. |
| etr_popupTime | *integer* | 0 | Delay (msec) between popups. |
| etr_popupStayTime | *integer* | 2000 | Time (msec) a popup stays on screen before fading out. |
| etr_popupFadeTime | *integer* | 2500 | Time (msec) a popup takes to fade out. |
| etr_popupGrouped | 0,1,2 | 2 | Group popups. Use 2 to disable console printouts for repeating popups. Introduced in ETrun 2.0.0. |
| etr_drawPopups | 0,1 | 2 | Draw popups. Introduced in ETrun 2.0.0. |
| etr_popupX | 0-640 | 4 | Popups horizontal position. Introduced in ETrun 2.0.0. |
| etr_popupY | 0-480 | 360 | Popups vertical position. Introduced in ETrun 2.0.0. |

# Pressed keys

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawKeys | 0,1,2,3 | 4 | Display pressed keys. Display various types according to value. |
| etr_keysX | 0-640 | 572 | Keys horizontal position. |
| etr_keysY | 0-480 | 210 | Keys vertical position. |
| etr_keysSize | *integer* | 64 | Control size of displayed keys. |

# Run timer

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawTimer | 0,1 | 1 | Display a run timer. |
| etr_timerX | 0-640 | 320 | Run timer horizontal position. |
| etr_timerY | 0-480 | 420 | Run timer vertical position. |
| etr_timerColor1 | *string* | White | Timer neutral color. Introduced in ETrun 2.0.0. |
| etr_timerColor2 | *string* | Green | Timer color for faster runs. Introduced in ETrun 2.0.0. |
| etr_timerColor3 | *string* | Red | Timer color for slower runs. Introduced in ETrun 2.0.0. |

# Scoreboard

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_countryFlags | 0,1 | 0 | Display country flags on scoreboard (server needs to support GeoIP). |

# Speclock

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_specLock | 0,1 | 0 | Enable persistant speclock. |

# Spectator state

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawSpectatorState | 0,1 | 1 | Draw spectator state. Introduced in ETrun 2.0.0. |
| etr_spectatorStateX | 0-640 | 320 | Spectator state horizontal position. Introduced in ETrun 2.0.0. |
| etr_spectatorStateY | 0-480 | 80 | Spectator state vertical position. Introduced in ETrun 2.0.0. |
| etr_spectatorStateColor | *string* | White | Spectator state color. Introduced in ETrun 2.0.0. |

Note: Spectator state also draws noclip state.

# Speed meter

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawSpeedMeter | 0,1 | 1 | Display a speed meter. |
| etr_speedMeterX | 0-640 | 320 | Speed meter horizontal position. |
| etr_speedMeterY | 0-480 | 220 | Speed meter vertical position. |
| etr_speedMeterColor1 | *string* | White | Speed meter neutral color. Introduced in ETrun 2.0.0. |
| etr_speedMeterColor2 | *string* | Green | Speed meter color for negative acceleration. Introduced in ETrun 2.0.0. |
| etr_speedMeterColor3 | *string* | Red | Speed meter color for positive acceleration. Introduced in ETrun 2.0.0. |
| etr_drawAccel | 0,1 | 0 | Color speed meter according to player acceleration. Introduced in ETrun 1.4.0. |
| etr_accelSmoothness | *integer* | 100 | Sensitivity of the `cg_drawAccel` cvar. Introduced in ETrun 1.4.0. |

# Statusline

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawStatusline | 0,1 | 1 | Draw demo statusline. Introduced in ETrun 2.0.0. |
| etr_statuslineX | 0-640 | 0 | Statusline horizontal position. Introduced in ETrun 2.0.0. |
| etr_statuslineY | 0-480 | 9 | Statusline vertical position. Introduced in ETrun 2.0.0. |
| etr_statuslineColor | *string* | White | Statusline color. Introduced in ETrun 2.0.0. |

# Surface detection

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawOB | 0,1 | 0 | Display OverBounce detector. |
| etr_OBX | 0-640 | 318 | OverBounce detector horizontal position. Introduced in ETrun 2.0.0. |
| etr_OBY | 0-480 | 230 | OverBounce detector vertical position. Introduced in ETrun 2.0.0. |
| etr_OBColor | *string* | White | OverBounce detector color. Introduced in ETrun 2.0.0. |
| etr_drawSlick | 0,1 | 0 | Display slick surface detector. |
| etr_slickX | 0-640 | 309 | Slick surface detector horizontal position. Introduced in ETrun 2.0.0. |
| etr_slickY | 0-480 | 230 | Slick surface detector vertical position. Introduced in ETrun 2.0.0. |
| etr_slickColor | *string* | White | Slick surface detector color. Introduced in ETrun 2.0.0. |

# Timeruns.net API authentication

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_authToken | *string* | "" | Player authentification token for timeruns.net API. |
| etr_autoLogin | 0,1 | 0 | Enable automatic player login. |

# Triggers visibility

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawTriggers | 0,1,2 | 1 | Make triggers visible using custom shaders. Use 0 to disable. Introduced in ETrun 1.4.0. |
| etr_triggersDrawScale | *float* | -0.5 | Apply a factor to the size of triggers. Introduced in ETrun 1.4.0. |
| etr_triggersDrawEdges | *float* | -0.5 | Make trigger edges visible. Introduced in ETrun 2.0.0. |

Possibles values for `etr_drawTriggers` are listed below.

| Value | What is displayed |
| ----- | ----------------- |
| 0 | None |
| 1 | ETrun Triggers |
| 2 | Debug (all Triggers) |

# Velocity Snapping HUD

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_drawVelocitySnapping | 0,1,2 | 0 | Draw Velocity Snapping HUD. Various types are available. Use 0 to disable. Introduced in ETrun 1.4.0. |
| etr_velocitySnappingH | *integer* | 8 | Velocity Snapping HUD height. Introduced in ETrun 1.4.0. |
| etr_velocitySnappingY | 0-480 | 240 | Velocity Snapping HUD vertical position. Introduced in ETrun 1.4.0. |
| etr_velocitySnappingFov | *integer* | 120 | Velocity Snapping Field Of View. Introduced in ETrun 1.4.0. |
| etr_velocitySnapping1Color1 | *string* | 0.4 0 0 0.5 | Velocity Snapping 1 primary color. Introduced in ETrun 2.0.0. |
| etr_velocitySnapping1Color2 | *string* | 0 0.4 0.4 0.5 | Velocity Snapping 1 secondary color. Introduced in ETrun 2.0.0. |
| etr_velocitySnapping2Color | *string* | White | Velocity Snapping 2 color. Introduced in ETrun 2.0.0. |

# Widescreen

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| etr_widescreenSupport | 0,1 | 1 | Enable widescreen display. Introduced in ETrun 1.4.0. |
| etr_realFov | 0,1 | 0 | Enable widescreen Field Of View. Introduced in ETrun 1.4.0. |
