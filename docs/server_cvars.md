List of ETrun server cvars.

Note: Some cvars use `bitflagged` value, it means you can combine options by making the sum of their related flags.

# Custom mapscripts

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_mapScriptDirectory | *string* | "custommapscripts" | Name of the custom mapscripts directory. |

# Enable/disable client features

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_disableCGaz | 0,1 | 0 | Disable usage of CGaz. Introduced in 1.4.0. |
| g_disableSnappingHUD | 0,1 | 0 | Disable usage of Velocity Snapping HUD. Introduced in 1.4.0. |

# Firewall

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_maxConnsPerIP | *integer* | 3 | Maximum clients allowed to connect from a same IP address. |

# Flood protection

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_floodProtect | 0,1 | 1 | Enable flood protection. |
| g_floodThreshold | *integer* | 8 | Limit the number of commands a client can send in an interval of 30 seconds. |
| g_floodWait | *integer* | 768 | Delay (msec) required between two commands from a client. |
| g_maxNameChanges | *integer* | 3 | Maximum name changes allowed per map. Use -1 to disable. |
| vote_delay | *integer* | 20000 | Force a delay (msec) between two votes. Introduced in 1.4.0. |

# Game physics

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| physics | *integer* | 255 | Set game physics according to the given **bitflagged** value. |

Available flags for `physics` are listed below.

| Flag | Value |
| ---- | ----- |
| Vanilla ET | 0 |
| Flat jumping | 1 |
| No fall damage | 2 |
| Ramp bounce | 4 |
| Air control | 8 |
| No overbounce | 16 |
| Upmove bugfix | 32 |
| Double jump | 64 |
| Slick control | 128 |

Sticky values for physics used on official timeruns.net game servers are listed below.

| Physics name | `physics` value |
| ------------ | --------------- |
| VET | 0 |
| VQ3 | 3 |
| VQ3 no OB | 19 |
| AP with OB | 239 |
| AP | 255 |

# GeoIP

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_useGeoIP | 0,1 | 0 | Enable use of GeoIP to geolocate and display country flags of players based on their IP addresses. |
| g_geoIPDbPath | *string* | "" | Path to GeoIP database inside `<fs_basepath>/etrun` directory. Compatible with [Maxmind Geolite2 Country](https://dev.maxmind.com/geoip/geoip2/geolite2/). |

# Logging

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_chatLog | 0,1 | 1 | Enable logging player chat to a separate `chat.log` file. Introduced in 1.2.0. |
| g_debugLog | 0,1 | 0 | Enable debug logging to a `debug.log` file. |

# Map entities

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_enableMapEntities | *integer* | 31 | Change game map entities behaviour according to the given **bitflagged** value. |
| g_forceTimerReset | 0,1 | 1 | Make sure start triggers are not prevented to reset run timer.
| g_holdDoorsOpen | 0,1 | 1 | Hold the doors open. |
| g_disableDrowning | 0,1 | 1 | Prevent player from drowning. |

Available flags for `g_enableMapEntities` are listed below.

| Flag | Value |
| ---- | ----- |
| Classic settings | 0 |
| Force kill entities to work | 1 |
| Force hurt entities to work | 2 |
| Enable jumppads `trigger_push` | 4 |
| Enable velocity jumppads `trigger_push_velocity` | 8 |
| Enable location jumppads `target_location` | 16 |
| Disable hurt entities | 32 |

# Strict save/load

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_strictSaveLoad | 0,1 | 0 | Enable [strict save/load mode](https://github.com/ETrun/ETrun/issues/41). |

# Timelimit

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| timelimit | *integer* | 0 | Amount of time before a random map gets loaded. 0 means no timelimit. Requires API. |

# Timeruns.net API

| Name  | Value | Default | Description |
| ----- | ----- | ------- | ----------- |
| g_useAPI | 0,1 | 0 | Enable use of timeruns.net API module. |
| g_APImoduleName | *string* | timeruns.mod | Name of API module file (must be located either in `fs_homepath` or `fs_basepath`). |
| g_cupMode | 0,1 | 0 | Enable cup mode. |
| g_cupKey | *string* | "" | Access key used while server is running in cup mode. |
