// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"

// Colors
vec4_t clrUiBack = { 0.f, 0.f, 0.f, .5f };
vec4_t clrUiYou  = { 0.2f, 0.1f, 0.1f, .5f };

#define INFO_PLAYER_WIDTH               75
#define INFO_TEAM_WIDTH                 40
#define INFO_SCORE_WIDTH                55
#define INFO_SPEED_WIDTH                40
#define INFO_LATENCY_WIDTH              35
#define INFO_STATE_WIDTH                60

#define INFO_TOTAL_WIDTH                (INFO_PLAYER_WIDTH + INFO_TEAM_WIDTH + INFO_SCORE_WIDTH + INFO_SPEED_WIDTH + INFO_LATENCY_WIDTH + INFO_STATE_WIDTH)

#define INFO_LINE_HEIGHT                12

#define INFO_SPEC_PLAYER_WIDTH          INFO_PLAYER_WIDTH
#define INFO_SPEC_FOLLOWED_PLAYER_WIDTH INFO_PLAYER_WIDTH
#define INFO_SPEC_SCORE_WIDTH           INFO_SCORE_WIDTH
#define INFO_SPEC_SPEED_WIDTH           INFO_SPEED_WIDTH
#define INFO_SPEC_LATENCY_WIDTH         INFO_LATENCY_WIDTH
#define INFO_SPEC_TOTAL_WIDTH           (INFO_SPEC_PLAYER_WIDTH + INFO_SPEC_FOLLOWED_PLAYER_WIDTH + INFO_SPEC_SCORE_WIDTH + INFO_SPEC_SPEED_WIDTH + INFO_SPEC_LATENCY_WIDTH)

#define NAME_MAX_LENGHT                 12

#define GOOD_PING_LIMIT                 100
#define MEDIUM_PING_LIMIT               200

#define NUM_PLAYERS_LAYOUT              8 // Nico, if there are more than 8 players, split the scoreboard in two columns

/* ETrun jump n newlines
 *
 * @author Nico
 */
static void WM_ETrun_newlines(int n, int *y, int newlineSize) {
	*y += newlineSize * n;
}

/* ETrun draw text utility
 *
 * @author Nico
 */
static void WM_ETrun_print(const char *s, fontInfo_t *font, float scale, int x, int y, qboolean shadowed, int maxlen) {
	vec4_t clrUiWhite = { 1.0f, 1.0f, 1.0f, 0.8f };

	if (shadowed == qtrue) {
		CG_Text_Paint_Ext(x, y, scale, scale, clrUiWhite, s, 0, maxlen, ITEM_TEXTSTYLE_SHADOWED, font);
	} else {
		CG_Text_Paint_Ext(x, y, scale, scale, clrUiWhite, s, 0, maxlen, 0, font);
	}
}

/* ETrun draw text utility
 *
 * @author Nico
 */
static void WM_ETrun_center_print(const char *s, fontInfo_t *font, float scale, int y, qboolean shadowed) {
	WM_ETrun_print(s, font, scale, SCREEN_WIDTH / 2 - CG_Text_Width_Ext(s, scale, 0, font) / 2, y, shadowed, 0);
}

/* Insert sort
 *
 * @author Nico
 */
static void insert_sort(s_timerunScores *tab, int size) {
	int i;
	int j;

	for (i = 1; i < size; ++i) {
		s_timerunScores elem = tab[i];
		for (j = i; j > 0 && tab[j - 1].timerunBestTime > elem.timerunBestTime; --j)
			tab[j] = tab[j - 1];
		tab[j] = elem;
	}
}

/* New ETrun color ping function
 *
 * @author Nico
 */
static char *WM_ETrun_coloredPing(int ping) {
	char *s;

	if (ping == -1) {
		s = va("-1");
	} else if (ping <= GOOD_PING_LIMIT) {
		s = va("^2%i", ping);
	} else if (ping <= MEDIUM_PING_LIMIT) {
		s = va("^3%i", ping);
	} else {
		s = va("^1%i", ping);
	}

	return s;
}

/* Draw country flags
 *
 * @source: ETpub
 */
#define COUNTRY_FLAG_RENDER_SIZE 16
#define COUNTRY_FLAG_INDIVIDUAL_SIZE 32
#define COUNTRY_FLAG_WIDTH 512
static qboolean WM_ETrun_drawCountryFlag(float x, float y, unsigned int countryCode) {
	float alpha[4];

	if (countryCode < 255) {
		float x1 = (float)((countryCode * (unsigned int)COUNTRY_FLAG_INDIVIDUAL_SIZE) % COUNTRY_FLAG_WIDTH);
		float y1 = (float)(floor((countryCode * COUNTRY_FLAG_INDIVIDUAL_SIZE) / COUNTRY_FLAG_WIDTH) * COUNTRY_FLAG_INDIVIDUAL_SIZE);
		float x2 = x1 + COUNTRY_FLAG_INDIVIDUAL_SIZE;
		float y2 = y1 + COUNTRY_FLAG_INDIVIDUAL_SIZE;
		alpha[0] = alpha[1] = alpha[2] = alpha[3] = 1.0;

		trap_R_SetColor(alpha);
		CG_DrawPicST(x, y - 12, COUNTRY_FLAG_RENDER_SIZE, COUNTRY_FLAG_RENDER_SIZE, x1 / COUNTRY_FLAG_WIDTH, y1 / COUNTRY_FLAG_WIDTH, x2 / COUNTRY_FLAG_WIDTH, y2 / COUNTRY_FLAG_WIDTH, cgs.media.worldFlags);
		trap_R_SetColor(NULL);
		return qtrue;
	}
	return qfalse;
}

/* New ETrun draw header function
 *
 * @author Nico
 */
#define MOD_LOGO_SIZE   128
static void WM_ETrun_DrawHeader(int *y, fontInfo_t *font) {
	const char *s;

	// ETrun x.x
	CG_DrawPic(SCREEN_WIDTH / 2 - MOD_LOGO_SIZE / 2, *y - MOD_LOGO_SIZE / 2, MOD_LOGO_SIZE, MOD_LOGO_SIZE, cgs.media.modLogo);

	WM_ETrun_newlines(4, y, SMALLCHAR_HEIGHT);

	// Map
	s = va("%s", va("Map: ^w%s", cgs.rawmapname));
	WM_ETrun_center_print(s, font, 0.25f, *y, qtrue);

	WM_ETrun_newlines(3, y, SMALLCHAR_HEIGHT);
}

/* New ETrun draw players function
 *
 * @author Nico
 */
static void WM_ETrun_DrawPlayers(int *x, int *y, fontInfo_t *font, s_timerunScores *orderedScores, int numScores) {
	char  *s;
	int   tempx = 0;
	float fontsize = 0.16f;
	int   i = 0;
	int   mil, min, sec;
	char  status[MAX_QPATH] = { 0 };

	// Draw "Players"
	s     = "Players";
	tempx = *x + INFO_TOTAL_WIDTH / 2  - CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1) / 2;
	WM_ETrun_print(s, font, 0.25f, tempx, *y, qtrue, 0);

	WM_ETrun_newlines(1, y, SMALLCHAR_HEIGHT);

	// Draw player info headings background

	CG_FillRect(*x, *y, INFO_TOTAL_WIDTH, INFO_LINE_HEIGHT, clrUiBack);
	CG_DrawRect_FixedBorder(*x, *y, INFO_TOTAL_WIDTH, INFO_LINE_HEIGHT, 1, colorBlack);
	*y += INFO_LINE_HEIGHT - 2;

	// Draw player info headings
	tempx = *x;
	WM_ETrun_print("Name", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_PLAYER_WIDTH;

	WM_ETrun_print("Team", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_TEAM_WIDTH;

	WM_ETrun_print("Time", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_SCORE_WIDTH;

	WM_ETrun_print("Speed", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_SPEED_WIDTH;

	WM_ETrun_print("Ping", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_LATENCY_WIDTH;

	WM_ETrun_print("Status", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_STATE_WIDTH;

	for (i = 0; i < numScores; ++i) {
		qboolean drawFlag = qfalse;

		// Ignore spectators
		if (orderedScores[i].team == TEAM_SPECTATOR) {
			continue;
		}

		tempx = *x;
		*y   += 4;

		// Draw background (highlight your line)
		CG_FillRect(*x, *y, INFO_TOTAL_WIDTH, INFO_LINE_HEIGHT, orderedScores[i].clientNum == cg.clientNum ? clrUiYou : clrUiBack);
		CG_DrawRect_FixedBorder(*x, *y, INFO_TOTAL_WIDTH, INFO_LINE_HEIGHT, 1, colorBlack);
		*y += INFO_LINE_HEIGHT - 2;

		// Nico, draw country flag
		if (cg_countryFlags.integer) {
			if (WM_ETrun_drawCountryFlag(tempx, *y, orderedScores[i].countryCode)) {
				tempx   += 16;
				drawFlag = qtrue;
			}
		}

		// Nico, draw player name
		WM_ETrun_print(orderedScores[i].name, font, fontsize, tempx, *y, qtrue, drawFlag == qfalse ? NAME_MAX_LENGHT : NAME_MAX_LENGHT - 3);
		tempx += drawFlag == qfalse ? INFO_PLAYER_WIDTH : INFO_PLAYER_WIDTH - 16;

		// Nico, draw team
		if (orderedScores[i].team == TEAM_AXIS) {
			s = "^1Axis";
		} else {
			s = "^4Allies";
		}
		WM_ETrun_print(s, font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_TEAM_WIDTH;

		// Nico, show best time
		if (orderedScores[i].timerunBestTime) {
			mil  = orderedScores[i].timerunBestTime;
			min  = mil / 60000;
			mil -= min * 60000;
			sec  = mil / 1000;
			mil -= sec * 1000;

			s = va("^7%02d:%02d.%03d", min, sec, mil);
		} else {
			s = "-";
		}
		WM_ETrun_print(s, font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_SCORE_WIDTH;

		// Nico, draw best speed
		WM_ETrun_print(va("%d", orderedScores[i].timerunBestSpeed), font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_SPEED_WIDTH;

		// Nico, draw ping
		WM_ETrun_print(WM_ETrun_coloredPing(orderedScores[i].ping), font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_LATENCY_WIDTH;

		// Nico, reset status
		memset(status, 0, sizeof (status));

		// Nico, draw status
		if (orderedScores[i].timerunStatus == 1) {
			Q_strcat(status, sizeof (status), "^2R ");
		}
		if (orderedScores[i].clientLogged == 1) {
			Q_strcat(status, sizeof (status), "^7L ");
		}
		if (orderedScores[i].clientCGaz == 1) {
			Q_strcat(status, sizeof (status), "^8C ");
		}
		if (orderedScores[i].clientHidden == 1) {
			Q_strcat(status, sizeof (status), "^9H ");
		}
		if (orderedScores[i].speclocked == 1) {
			Q_strcat(status, sizeof (status), "^bS ");
		}
		if (status[0] == '\0') {
			Q_strncpyz(status, "-", sizeof (status));
		}

		WM_ETrun_print(status, font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_STATE_WIDTH;
	}
	WM_ETrun_newlines(2, y, SMALLCHAR_HEIGHT);
}

/* New ETrun draw spectators function
 *
 * @author Nico
 */
static void WM_ETrun_DrawSpectators(int *x, int *y, fontInfo_t *font, s_timerunScores *orderedScores, int numScores) {
	char  *s;
	int   tempx = 0;
	float fontsize = 0.16f;
	int   i = 0;
	int   mil, min, sec;

	// Draw "Spectators"
	s     = "Spectators";
	tempx = *x + INFO_SPEC_TOTAL_WIDTH / 2  - CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1) / 2;
	WM_ETrun_print(s, font, 0.25f, tempx, *y, qtrue, 0);

	WM_ETrun_newlines(1, y, SMALLCHAR_HEIGHT);

	// Draw player info headings background
	CG_FillRect(*x, *y, INFO_SPEC_TOTAL_WIDTH, INFO_LINE_HEIGHT, clrUiBack);
	CG_DrawRect_FixedBorder(*x, *y, INFO_SPEC_TOTAL_WIDTH, INFO_LINE_HEIGHT, 1, colorBlack);
	*y += INFO_LINE_HEIGHT - 2;

	// Draw spec info headings
	tempx = *x;
	WM_ETrun_print("Name", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_SPEC_PLAYER_WIDTH;

	WM_ETrun_print("Following", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_SPEC_FOLLOWED_PLAYER_WIDTH;

	WM_ETrun_print("Time", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_SPEC_SCORE_WIDTH;

	WM_ETrun_print("Speed", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_SPEC_SPEED_WIDTH;

	WM_ETrun_print("Ping", font, fontsize, tempx, *y, qtrue, 0);
	tempx += INFO_SPEC_LATENCY_WIDTH;

	for (i = 0; i < numScores; ++i) {
		qboolean drawFlag = qfalse;

		// Ignore non-spectators
		if (orderedScores[i].team != TEAM_SPECTATOR) {
			continue;
		}

		tempx = *x;
		*y   += 4;

		// Draw background (highlight your line)
		CG_FillRect(*x, *y, INFO_SPEC_TOTAL_WIDTH, INFO_LINE_HEIGHT, orderedScores[i].clientNum == cg.clientNum ? clrUiYou : clrUiBack);
		CG_DrawRect_FixedBorder(*x, *y, INFO_SPEC_TOTAL_WIDTH, INFO_LINE_HEIGHT, 1, colorBlack);
		*y += INFO_LINE_HEIGHT - 2;

		// Nico, draw country flag
		if (cg_countryFlags.integer) {
			if (WM_ETrun_drawCountryFlag(tempx, *y, orderedScores[i].countryCode)) {
				tempx   += 16;
				drawFlag = qtrue;
			}
		}

		// Nico, draw player name
		WM_ETrun_print(orderedScores[i].name, font, fontsize, tempx, *y, qtrue, drawFlag == qfalse ? NAME_MAX_LENGHT : NAME_MAX_LENGHT - 3);
		tempx += drawFlag == qfalse ? INFO_PLAYER_WIDTH : INFO_PLAYER_WIDTH - 16;

		// Nico, draw followed client name
		if (orderedScores[i].clientNum != orderedScores[i].followedClient) {
			s = orderedScores[i].followedClientName;
		} else {
			s = "-";
		}
		WM_ETrun_print(s, font, fontsize, tempx, *y, qtrue, NAME_MAX_LENGHT);
		tempx += INFO_SPEC_FOLLOWED_PLAYER_WIDTH;

		// Nico, show best time
		if (orderedScores[i].timerunBestTime) {
			mil  = orderedScores[i].timerunBestTime;
			min  = mil / 60000;
			mil -= min * 60000;
			sec  = mil / 1000;
			mil -= sec * 1000;

			s = va("^7%02d:%02d.%03d", min, sec, mil);
		} else {
			s = "-";
		}
		WM_ETrun_print(s, font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_SPEC_SCORE_WIDTH;

		// Nico, draw best speed
		WM_ETrun_print(va("%d", orderedScores[i].timerunBestSpeed), font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_SPEC_SPEED_WIDTH;

		// Nico, draw ping
		WM_ETrun_print(WM_ETrun_coloredPing(orderedScores[i].ping), font, fontsize, tempx, *y, qtrue, 0);
		tempx += INFO_SPEC_LATENCY_WIDTH;
	}
}

/* ETrun draw scoreboard info like clock, physics and mod version
 *
 * @author Nico
 */
#define SB_INFO_X       550
#define SB_INFO_Y       9
static void WM_ETrun_DrawInfo() {
	int   x                      = SB_INFO_X;
	int   y                      = SB_INFO_Y;
	char  physicsDesc[MAX_QPATH] = { 0 };
	float textScale              = 0.12f;

	// Nico, draw time
	CG_DrawScoresClock(x, y, textScale);

	// Nico, draw physics
	getPhysicsDesc(physicsDesc, physics.integer);

	y += 10;
	WM_ETrun_print(va("Physics: %s", physicsDesc), &cgs.media.limboFont1, textScale, x, y, qtrue, 0);
#include "../game/bg_version.h"
	y += 10;
	// Nico, print mod version
	WM_ETrun_print(va("Version: %s", MOD_VERSION), &cgs.media.limboFont1, textScale, x, y, qtrue, 0);
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboard(void) {
	int             x = 0;
	int             y = 0;
	s_timerunScores orderedScores[MAX_CLIENTS];
	int             i         = 0;
	int             j         = 0;
	int             numScores = cg.numScores;
	int             teamPlayers[TEAM_NUM_TEAMS];

	// don't draw anything if the menu or console is up
	if (cg_paused.integer) {
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	// OSP - also for pesky scoreboards in demos
	if (!cg.showScores) {
		return qfalse;
	}

	// don't draw if in cameramode
	if (cg.cameraMode) {
		return qtrue;
	}

	// Nico, update ordered scores

	// Fill players array
	for (i = 0; i < numScores; ++i) {
		orderedScores[i].scoreId   = i;
		orderedScores[i].clientNum = cg.scores[i].client;
		Q_strncpyz(orderedScores[i].name, cgs.clientinfo[cg.scores[i].client].name, MAX_QPATH);
		orderedScores[i].team             = cg.scores[i].team;
		orderedScores[i].timerunBestTime  = cg.scores[i].timerunBestTime; // Best time
		orderedScores[i].timerunBestSpeed = cg.scores[i].timerunBestSpeed; // Best speed
		orderedScores[i].timerunStatus    = cg.scores[i].timerunStatus; // Timerun status
		orderedScores[i].clientLogged     = cg.scores[i].logged; // Client login status
		orderedScores[i].clientCGaz       = cg.scores[i].cgaz; // Client cgaz setting
		orderedScores[i].clientHidden     = cgs.clientinfo[cg.scores[i].client].hideme; // Client hideme
		orderedScores[i].ping             = cg.scores[i].ping;
		orderedScores[i].followedClient   = cg.scores[i].followedClient; // Followed client
		Q_strncpyz(orderedScores[i].followedClientName, cgs.clientinfo[cg.scores[i].followedClient].name, MAX_QPATH); // Followed client name
		orderedScores[i].speclocked  = cg.scores[i].speclocked; // Speclock status
		orderedScores[i].countryCode = cgs.clientinfo[cg.scores[i].client].countryCode;
	}

	// Nico, fake test data
#if 0
	numScores = 24;
	srand(time(NULL));
	for (j = i; j < numScores; ++j) {
		orderedScores[j].scoreId   = j;
		orderedScores[j].clientNum = j;
		Q_strncpyz(orderedScores[j].name, "Fake client", MAX_QPATH);
		orderedScores[j].team             = rand() % 3 + 1;
		orderedScores[j].timerunBestTime  = rand() % 90000; // Best time
		orderedScores[j].timerunBestSpeed = rand() % 3000; // Best speed
		orderedScores[j].timerunStatus    = rand() % 2; // Timerun status
		orderedScores[i].clientLogged     = 0; // Client login status
		orderedScores[i].clientCGaz       = 0; // Client cgaz setting
		orderedScores[j].ping             = rand() % 800;
		orderedScores[j].followedClient   = 0; // Followed client
		Q_strncpyz(orderedScores[j].followedClientName, "none", MAX_QPATH); // Followed client name
		orderedScores[i].countryCode = rand() % 255;
	}
#endif

	// Sort it
	insert_sort(orderedScores, numScores);

	// Put 00:00.00 at end of array
	i = 0;
	while ((i < numScores) && (orderedScores[i].timerunBestTime == 0)) {
		for (j = i; j < numScores; ++j) {
			if (orderedScores[j].timerunBestTime != 0) {
				s_timerunScores temp = orderedScores[j];
				orderedScores[j] = orderedScores[i];
				orderedScores[i] = temp;
				break;
			}
		}
		i++;
	}

	// Count team players
	teamPlayers[TEAM_AXIS]      = 0;
	teamPlayers[TEAM_ALLIES]    = 0;
	teamPlayers[TEAM_SPECTATOR] = 0;
	for (i = 0; i < numScores; ++i) {
		teamPlayers[orderedScores[i].team]++;
	}

	// Nico, draw info like mod version, clock and physics
	WM_ETrun_DrawInfo();

	// Nico, draw scoreboard header
	y = 30; // Start drawing from y = 30
	WM_ETrun_DrawHeader(&y, &cgs.media.limboFont1);

	if (numScores < NUM_PLAYERS_LAYOUT) {
		// Nico, single column scoreboard

		WM_ETrun_newlines(3, &y, SMALLCHAR_HEIGHT);

		// Nico, draw players, if any
		x = (SCREEN_WIDTH - INFO_TOTAL_WIDTH) / 2; // Nico, center horizontally
		if (teamPlayers[TEAM_ALLIES] != 0 || teamPlayers[TEAM_AXIS] != 0) {
			WM_ETrun_DrawPlayers(&x, &y, &cgs.media.limboFont1, orderedScores, numScores);
		}

		// Nico, draw spectators, if any
		x = (SCREEN_WIDTH - INFO_SPEC_TOTAL_WIDTH) / 2; // Nico, center horizontally
		if (teamPlayers[TEAM_SPECTATOR] != 0) {
			WM_ETrun_DrawSpectators(&x, &y, &cgs.media.limboFont1, orderedScores, numScores);
		}
	} else { // Nico, 2-columns scoreboard
		qboolean thereArePlayers = qfalse, thereAreSpectators = qfalse;

		if (teamPlayers[TEAM_ALLIES] != 0 || teamPlayers[TEAM_AXIS] != 0) {
			thereArePlayers = qtrue;
		}

		if (teamPlayers[TEAM_SPECTATOR] != 0) {
			thereAreSpectators = qtrue;
		}

		if (thereArePlayers && thereAreSpectators) { // Nico, 2-columns
			int yCopy = y;

			// Nico, draw players on a first column
			x = (SCREEN_WIDTH / 2 - INFO_TOTAL_WIDTH) / 2;     // Nico, center horizontally
			WM_ETrun_DrawPlayers(&x, &y, &cgs.media.limboFont1, orderedScores, numScores);

			// Nico, draw spectators on a second column
			x = SCREEN_WIDTH - INFO_SPEC_TOTAL_WIDTH - (SCREEN_WIDTH / 2 - INFO_SPEC_TOTAL_WIDTH) / 2; // Nico, center horizontally
			y = yCopy;
			WM_ETrun_DrawSpectators(&x, &y, &cgs.media.limboFont1, orderedScores, numScores);

		} else if (thereArePlayers == qtrue) {
			// Nico, 1-column of players
			x = (SCREEN_WIDTH - INFO_TOTAL_WIDTH) / 2; // Nico, center horizontally
			WM_ETrun_DrawPlayers(&x, &y, &cgs.media.limboFont1, orderedScores, numScores);
		} else {
			// Nico, 1-column of spectators
			x = (SCREEN_WIDTH - INFO_SPEC_TOTAL_WIDTH) / 2; // Nico, center horizontally
			WM_ETrun_DrawSpectators(&x, &y, &cgs.media.limboFont1, orderedScores, numScores);
		}

	}

	return qtrue;
}
