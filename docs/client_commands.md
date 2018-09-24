There are various client commands available in ETrun. They are described below.

# save

```
/save [slotId]
```

Save current position into the given `slotId`. If `slotId` is omitted, default to slot 0.

# load

```
/load [slotId]
```

Load a previously saved position from saved slots. If `slotId` is omitted, default to slot 0.

- View angles are also loaded if `cg_loadViewAngles = 1`.
- Weapon is also loaded if `cg_loadWeapon = 1`.

# login

```
/login
```

Attempt to login the player on timeruns.net API using `cg_timerunsToken` value.

# logout

```
/logout
```

Logout a player from timeruns.net API.

# records

```
/records
```

Request timeruns.net API for records of every runs in the current map and print them.

# loadCheckpoints

```
/loadCheckpoints [userName] [runName]
/loadCheckpoints [userName] [runId]
```

Request timeruns.net API for the run checkpoints of a user and load them.

- If `userName` is provided, it will look for checkpoints from a player named `userName`. If omitted, checkpoints from the current player will be loaded.
- If a second argument is omitted, this command will load checkpoints for the first run of the map. If provided, the command will:
    1. Try to load checkpoints for this specific run name.
    2. In case of failure it will to load records from a specific run by its numeric id.
    3. Finally, it will load records for the first run of the map.

*Examples:*

```
/loadCheckpoints player run2
/loadCheckpoints other_player
/loadCheckpoints other_player run2
```

# h (help)

```
/h [command]
```

Provide help about ETrun commands. If `command` is omitted, list all available commands. If provided, display usage of `command`.

# tutorial

```
/tutorial
```

Show an introduction for beginners into the console.

# rank

```
/rank [userName] [mapName] [runName] [physicsName]
```

List surrounding records time and ranks from -5 to +5.

When no option provided, this command will requires player to be logged in and will give his rank on the first run of current map with current physics settings. Otherwise, it will give output according to given options:

- `userName` name of the user to show record of.
- `mapName` name of the map to show record of.
- `runName` name of the run among `mapName` to show record of.
- `physicsName` name of the game physics to show record of. Possible values are **VET**,**VQ3** or **AP**.

*Examples:*

```
/rank player
/rank player shorties "Shorties 2"
/rank other_player shorties "Shorties 2" AP
```

# class

```
/class class [weapon1] [weapon2]
```

Change player class to `class`. Possible classes are described below.

- `weapon1` is the numeric id of the weapon among `class`.
- `weapon2` is the numeric id of alternate weapon among `class`.

| Class name  | `class` |
| ----------- | --------|
| Medic       | m       |
| Engineer    | e       |
| Field ops   | f       |
| Covert ops  | c       |
| Soldier     | s       |

# speclock

```
/speclock
```

Prevent spectators to spectate you.

# specunclock

```
/specunclock
```

Allow spectators to spectate you.

# specinvite

```
/specinvite playerId
```

Allow `playerId` to spectate you.

# specuninvite

```
/specuninvite playerId
```

Prevent a previously invited spectator to spectate you.

# m (private messaging)

```
/m playerName message
/m playerId message
```

Send a private message containing text  `message` to player name `playerName`. If `playerName` is not found, numeric player id `playerId` will be used instead.

# abort

```
/abort
```

Abort an active run.

# mod_information

```
/mod_information
```

Displays informations about mod.
