# ETrun

Welcome to the official ETrun documentation.

## What is ETrun?

ETrun is a Wolfenstein: Enemy Territory game modification based on ET-GPL. The objective of this mod is to bring timeruns support to it.

## Official website

[https://timeruns.net](https://timeruns.net)

## Downloading ETrun

Find all the releases on [GitHub releases page](https://github.com/ETrun/ETrun/releases).

## Get started

The goal is to get from the start of the map to the end as fast as possible. These timeruns are recorded to the official website where you can compare them to other players as long a you have registered a timeruns token. [Here](https://www.youtube.com/watch?v=g7gJ4YmnbLU) is an example of a timerun.

## Timeruns token

In order to permanently save records, you need to create an account on the official ETrun website, and link it to your game. Here is a step-by-step tutorial:

* Go to the [website](https://timeruns.net/) and open the Signup tab.
* Follow the instructions and wait for the account activation email.
* Once your account has been activated, login on the ETrun [forum](https://forum.timeruns.net/).
* In the top right corner, click on your nickname and follow this path:
  * User Control Panel
  * Profile
  * Edit account settings
* Now you can see your Timeruns token. This is your password which links your game to your own website account. Never share it!
* Copy your Timeruns token.
* Insert your Timeruns token ingame into the `/etr_authToken` cvar.
* Type `/login` into the console.

Congratulations! You are now logged in and able to set records. You cannow find your stats on the website and share them with your friends.

## Things to know

* **RGBA Color suppport**

ETrun color cvars support RGB(A) formats. The following formats are supported:

| Format | Example |
| ------ | ------- |
| string | White, Green, Blue |
| normalized RGB | 1.0 0 1.0 |
| normalized RGBA | 0.3 0 1.0 0.5 |
| true RGB | 0 255 63 |
| true RGBA | 83 159 230 100 |

* **HUD coordinates**

Most HUD elements can be moved by using respective cvars. Because of the widescreen support, all those elements are projected onto a virtual 640 x 480 screen and then scaled up to the client's resolution.

| Cvar | Example Value |
| ---- | ------------- |
| cg_chatX | 130 |
| etr_timerY | 420 |

All HUD elements cvars accept float values to guarantee precise adjustments for all ratios.

| Cvar | Example Value |
| ---- | ------------- |
| etr_keysX | 553.2 |
| etr_speedMeterY | 215.9 |

Some HUD elements are meant to be aligned to the screen edge. Not to break compability with widescreens, those cvar values define an offset and thus are relative, in comparison to absolute coordinate cvars.

| Cvar | Example Value |
| ---- | ------------- |
| etr_infoPanelXoffset | -10.2 |
| cg_playerHealthBarYoffset | 14 |

* **Restricted Cvars**

Certain cvars are either partly or completely restricted to prevent abuse. ETrun checks for bad cvar values and prevents playing with them automatically.

| Cvar | Allowed Values |
| ---- | ------------- |
| pmove_fixed | 1 |
| rate | 5000 - 32000 |
| cl_yawspeed | 0 |
