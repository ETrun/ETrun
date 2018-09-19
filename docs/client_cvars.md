List of ETrun cvars.

# Speed meter

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawSpeedMeter | 0,1 | 1 | Display a speed meter. |
| cg_speedMeterX | 0-640 | 320 | Speed meter horizontal position. |
| cg_speedMeterY | 0-480 | 220 | Speed meter vertical position. |

# CGaz

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawCGaz | 0,1,2,3,4 | 0 | Draw CGaz. Various types are available. Use 0 to disable. |

# Run timer

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawTimer | 0,1 | 1 | Display a run timer. |
| cg_timerX | 0-640 | 320 | Run timer horizontal position. |
| cg_timerY | 0-480 | 420 | Run timer vertical position. |

# Checkpoints

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawCheckPoints | 0,1 | 1 | Display checkpoints times. |
| cg_checkPointsX | 0-640 | 320 | Checkpoints horizontal position. |
| cg_checkPointsY | 0-480 | 435 | Checkpoints vertical position. |
| cg_maxCheckPoints | *integer* | 5 | Maximum number of checkpoints displayed. |
| cg_autoLoadCheckpoints | 0,1 | 0 | Automatically load checkpoints from PB. Player must be logged in. |

# Hide player model

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_hideOthers | 0,1 | 1 | Hide other player models. |
| cg_hideRange | *integer* | 128 | Distance expressed in game units where other player models should be hidden. |
| cg_hideMe | 0,1 | 0 | Hide your player model from other players. |

# Speclock

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_specLock | 0,1 | 0 | Enable persistant speclock. |

# Display pressed keys

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawKeys | 0,1,2,3 | 1 | Display pressed keys. Display various types according to value. |
| cg_keysX | 0-640 | 550 | Keys horizontal position. |
| cg_keysY | 0-480 | 210 | Keys vertical position. |
| cg_keysSize | *integer* | 64 | Control size of displayed keys. |

# Timeruns.net API authentication

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_timerunsToken | *string* | "" | Player authentification token for timeruns.net API. |
| cg_autoLogin | 0,1 | 0 | Enable automatic player login. |

# Popups

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_numPopups | 0-16 | 5 | Maximum number of popups to display. |
| cg_popupTime | *integer* | 1000 | Delay (msec) between popups. |
| cg_popupStayTime | *integer* | 2000 | Time (msec) a popup stays on screen before fading out. |
| cg_popupFadeTime | *integer* | 2500 | Time (msec) a popup takes to fade out. |

# Info panel

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawInfoPanel | 0,1 | 1 | Display info panel. |
| cg_infoPanelX | 0-640 | 537 | Info panel horizontal position. |
| cg_infoPanelY | 0-480 | 2 | Info panel vertical position. |
| cg_minStartSpeed | *integer* | 0 | Minimum start speed to trigger red color in info panel. Default value is 0. Use 0 to disable. Introduced in ETrun 1.2.0. |

# Demos

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_autoDemo | 0,1 | 0 | Enable recording demos automatically. |
| cg_keepAllDemos | 0,1 | 1 | Keep all demos or only demos for PB records. |

# Player position control

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_loadViewAngles | 0,1 | 1 | Enable restoring player view angles when loading its position. |
| cg_loadWeapon | 0,1 | 1 | Enable restoring player weapon when loading its position. |
| cg_autoLoad | 0,1 | 1 | Enable automatically restoring player position when he gets killed. |

# Surface detection

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_drawOB | 0,1 | 0 | Display OverBounce detector. |
| cg_drawSlick | 0,1 | 0 | Display slick surface detector. |

# Logs

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_viewLog | 0,1 | 0 | Enable displaying the game console in a separate window from the main game window. |

# Noclip

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_noclipSpeed | *integer* | 1000 | Set movement speed while using `/noclip` command. |

# Scoreboard

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| cg_countryFlags | 0,1 | 0 | Display country flags on scoreboard (server needs to support GeoIP). |
