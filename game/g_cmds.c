/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"
#include "m_player.h"


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		sprintf(st, "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}


qboolean isnum(char* s)
{
	if (*s == '\0')
		return false;

	while (*s != '\0')
	{
		if ( !isdigit(*s) )
			return false;
		s++;
	}
	
	return true;
}

int Fetch_Stat_Total(gclient_t* client)
{
	int stat_total;

	stat_total = client->pers.str;
	stat_total += client->pers.end;
	stat_total += client->pers.per;
	stat_total += client->pers.intel;
	stat_total += client->pers.agl;
	stat_total += client->pers.lck;

	return stat_total;
}

void Cmd_Set_Special(edict_t* ent)
{
	char* name;
	int stats[6], i, sum;
	char* stat_names[] = { "Strength", "Perception", "Endurance", "Intelligence", "Agility", "Luck" };

	if (gi.argc() != 7)
	{
		gi.cprintf(ent, PRINT_HIGH, "This command demands 6 numerical parameters.\n");
		return;
	}

	if (ent->client->pers.stats_finished)
	{
		gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats have already been assigned.\n");
		return;
	}

	sum = 0;

	for (i = 0; i < 6; i++)
	{
		name = gi.argv(i+1);

		if (!isnum(name))
		{
			gi.cprintf(ent, PRINT_HIGH, "Invalid argument %d provided: '%s'\n", i, name);
			return;
		}

		stats[i] = atoi(name);

		if (stats[i] > 10 || stats[i] < 1)
		{
			gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats can be no less than 10 and no greater than 1.\n");
			return;
		}

		sum += stats[i];
	}
	
	if(sum > ent->client->pers.sp)
	{
		gi.cprintf(ent, PRINT_HIGH, "Not enough SPECIAL points for this allocation.\nYou are %d SPECIAL points over budget.\n", sum - ent->client->pers.sp);
		return;
	}
	
	ent->client->pers.str = stats[0];
	ent->client->pers.per = stats[1];
	ent->client->pers.end = stats[2];
	ent->client->pers.intel = stats[3];
	ent->client->pers.agl = stats[4];
	ent->client->pers.lck = stats[5];

	for (i = 0; i < 6; i++)
	{
		gi.cprintf(ent, PRINT_HIGH, "%s has been set to %d.\n", stat_names[i], stats[i]);
	}

	gi.cprintf(ent, PRINT_HIGH, "You have %d SPECIAL points remaining.\n", ent->client->pers.sp - sum);
}

void Cmd_Finish_Special(edict_t* ent)
{
	ent->client->pers.stats_finished = true;

	ent->client->pers.lvl = 1;
	ent->client->pers.next_lvl = 1000;
	ent->client->pers.sp = 0;

	UpdatePlayerSkills(ent);

	gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats have been assigned.\nYou may now begin playing.\n");
}

void Cmd_Set_Stat(edict_t* ent, int *stat, char *stat_name)
{
	int old, new, sum;
	char* name;

	//Check if stats already assigned.
	if (ent->client->pers.stats_finished)
	{
		gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats have already been assigned.\n");
		return;
	}

	name = gi.args();

	//Check if numerical.
	if ( !isnum(name) )
	{
		gi.cprintf(ent, PRINT_HIGH, "Invalid argument provided: '%s'\n", name);
		return;
	}

	new = atoi(name);

	if( new > 10 || new < 1 )
	{
		gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats can be no less than 10 and no greater than 1.\n");
		return;
	}

	old = *stat;
	*stat = new;
	sum = (Fetch_Stat_Total(ent->client));

	//Check if more stats needed than assigned.
	if( sum > ent->client->pers.sp)
	{
		*stat = old;
		gi.cprintf(ent, PRINT_HIGH, "Not enough SPECIAL points to set the stat to %d.\nYou have %d SPECIAL points remaining.\n", new, ent->client->pers.sp - sum );
		return;
	}

	gi.cprintf(ent, PRINT_HIGH, "%s has been set to %d.\n", stat_name, new);
	gi.cprintf(ent, PRINT_HIGH, "You have %d SPECIAL points remaining.\n", ent->client->pers.sp - sum);
}

void Cmd_Max_Stone(edict_t* ent)
{
	if (ent->client->pers.stats_finished)
	{
		gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats have already been assigned.\n");
		return;
	}

	ent->client->pers.str = 8;
	ent->client->pers.per = 4;
	ent->client->pers.end = 9;
	ent->client->pers.intel = 3;
	ent->client->pers.agl = 7;
	ent->client->pers.lck = 4;

	gi.cprintf(ent, PRINT_HIGH, "You are Max Stone.\n");

	Cmd_Finish_Special(ent);
}

void Cmd_Albert_Cole(edict_t* ent)
{
	if (ent->client->pers.stats_finished)
	{
		gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats have already been assigned.\n");
		return;
	}
	
	ent->client->pers.str = 5;
	ent->client->pers.per = 8;
	ent->client->pers.end = 4;
	ent->client->pers.intel = 6;
	ent->client->pers.agl = 4;
	ent->client->pers.lck = 8;

	gi.cprintf(ent, PRINT_HIGH, "You are Albert Cole.\n");

	Cmd_Finish_Special(ent);
}

void Cmd_Natalia(edict_t* ent)
{
	if (ent->client->pers.stats_finished)
	{
		gi.cprintf(ent, PRINT_HIGH, "SPECIAL stats have already been assigned.\n");
		return;
	}
	
	ent->client->pers.str = 3;
	ent->client->pers.per = 5;
	ent->client->pers.end = 5;
	ent->client->pers.intel = 7;
	ent->client->pers.agl = 9;
	ent->client->pers.lck = 6;

	gi.cprintf(ent, PRINT_HIGH, "You are Natalia Dubrovhsky.\n");

	Cmd_Finish_Special(ent);
}

void Cmd_Level_Up(edict_t* ent)
{
	if (ent->client->pers.exp < ent->client->pers.next_lvl)
		ent->client->pers.exp = ent->client->pers.next_lvl;
	
	ent->client->pers.lvl++;
	ent->client->pers.next_lvl += ent->client->pers.lvl * 1000;
	ent->client->pers.sp += 5 + (2 * ent->client->pers.intel);
	gi.bprintf(PRINT_HIGH, "YOU HAVE LEVELED UP!\n");

	if (ent->client->pers.lvl % 3 == 0)
	{
		ent->client->pers.perks += 1;
		gi.bprintf(PRINT_HIGH, "NEW PERKS AVAILABLE!\n");
	}

	
}

void Cmd_Level_Skill(edict_t* ent, int* stat, char* stat_name)
{
	int increase;
	char* name;

	//Check if player has skill points.
	if (ent->client->pers.sp == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "You do have no skill points to spend.\n");
		return;
	}

	name = gi.args();

	//Check if numerical.
	if (!isnum(name))
	{
		gi.cprintf(ent, PRINT_HIGH, "Invalid argument provided: '%s'\n", name);
		return;
	}

	increase = atoi(name);

	if (increase < 0 )
	{
		gi.cprintf(ent, PRINT_HIGH, "You cannot allocate negative skill points.\n");
		return;
	}

	if (increase > ent->client->pers.sp)
	{
		gi.cprintf(ent, PRINT_HIGH, "You do not have %i skill points to allocate.\nAllocating %i skill points instead.\n", increase, ent->client->pers.sp);
		increase = ent->client->pers.sp;
	}

	*stat += increase;
	ent->client->pers.sp -= increase;

	gi.cprintf(ent, PRINT_HIGH, "%s has been increased by %d to %d.\n", stat_name, increase, *stat);
	gi.cprintf(ent, PRINT_HIGH, "You have %d skill points remaining.\n", ent->client->pers.sp);
}

void Cmd_Unlock_Perk(edict_t* ent, qboolean* perk, char* perk_name)
{
	//Check if player has the perk already.
	if (*perk)
	{
		gi.cprintf(ent, PRINT_HIGH, "You already have the perk %S.\n", perk_name);
		return;
	}


	if (ent->client->pers.perks <= 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not able to unleck a perk at this time.\n");
		return;
	}

	*perk = true;

	gi.cprintf(ent, PRINT_HIGH, "You have unlocked the perk %s.\n", perk_name);
}

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	if (Q_stricmp(cmd, "stats") == 0)
	{
		Cmd_Stat_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp(cmd, "use") == 0)
		Cmd_Use_f(ent);
	else if (Q_stricmp(cmd, "drop") == 0)
		Cmd_Drop_f(ent);
	else if (Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if (Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if (Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if (Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	else if (Q_stricmp(cmd, "inven") == 0)
		Cmd_Inven_f(ent);
	else if (Q_stricmp(cmd, "invnext") == 0)
		SelectNextItem(ent, -1);
	else if (Q_stricmp(cmd, "invprev") == 0)
		SelectPrevItem(ent, -1);
	else if (Q_stricmp(cmd, "invnextw") == 0)
		SelectNextItem(ent, IT_WEAPON);
	else if (Q_stricmp(cmd, "invprevw") == 0)
		SelectPrevItem(ent, IT_WEAPON);
	else if (Q_stricmp(cmd, "invnextp") == 0)
		SelectNextItem(ent, IT_POWERUP);
	else if (Q_stricmp(cmd, "invprevp") == 0)
		SelectPrevItem(ent, IT_POWERUP);
	else if (Q_stricmp(cmd, "invuse") == 0)
		Cmd_InvUse_f(ent);
	else if (Q_stricmp(cmd, "invdrop") == 0)
		Cmd_InvDrop_f(ent);
	else if (Q_stricmp(cmd, "weapprev") == 0)
		Cmd_WeapPrev_f(ent);
	else if (Q_stricmp(cmd, "weapnext") == 0)
		Cmd_WeapNext_f(ent);
	else if (Q_stricmp(cmd, "weaplast") == 0)
		Cmd_WeapLast_f(ent);
	else if (Q_stricmp(cmd, "kill") == 0)
		Cmd_Kill_f(ent);
	else if (Q_stricmp(cmd, "putaway") == 0)
		Cmd_PutAway_f(ent);
	else if (Q_stricmp(cmd, "wave") == 0)
		Cmd_Wave_f(ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else if (Q_stricmp(cmd, "max_stone") == 0)
		Cmd_Max_Stone(ent);
	else if (Q_stricmp(cmd, "albert_cole") == 0)
		Cmd_Albert_Cole(ent);
	else if (Q_stricmp(cmd, "natalia") == 0)
		Cmd_Natalia(ent);
	else if (Q_stricmp(cmd, "set_special") == 0)
		Cmd_Set_Special(ent);
	else if (Q_stricmp(cmd, "finish_special") == 0)
		Cmd_Finish_Special(ent);
	else if (Q_stricmp(cmd, "set_str") == 0)
		Cmd_Set_Stat(ent, &(ent->client->pers.str), "Strength");
	else if (Q_stricmp(cmd, "set_per") == 0)
		Cmd_Set_Stat(ent, &(ent->client->pers.per), "Perception");
	else if (Q_stricmp(cmd, "set_end") == 0)
		Cmd_Set_Stat(ent, &(ent->client->pers.end), "Endurance");
	else if (Q_stricmp(cmd, "set_int") == 0)
		Cmd_Set_Stat(ent, &(ent->client->pers.intel), "Intelligence");
	else if (Q_stricmp(cmd, "set_agl") == 0)
		Cmd_Set_Stat(ent, &(ent->client->pers.agl), "Agility");
	else if (Q_stricmp(cmd, "set_lck") == 0)
		Cmd_Set_Stat(ent, &(ent->client->pers.lck), "Luck");
	else if (Q_stricmp(cmd, "cheat_level_up") == 0)
		Cmd_Level_Up(ent);
	else if (Q_stricmp(cmd, "level_sg") == 0)
		Cmd_Level_Skill(ent, &(ent->client->pers.small_guns), "Small Guns");
	else if (Q_stricmp(cmd, "level_bg") == 0)
		Cmd_Level_Skill(ent, &(ent->client->pers.big_guns), "Big Guns");
	else if (Q_stricmp(cmd, "level_ew") == 0)
		Cmd_Level_Skill(ent, &(ent->client->pers.energy_wep), "Energy Weapons");
	else if (Q_stricmp(cmd, "level_tr") == 0)
		Cmd_Level_Skill(ent, &(ent->client->pers.traps), "Traps");
	else if (Q_stricmp(cmd, "level_fa") == 0)
		Cmd_Level_Skill(ent, &(ent->client->pers.first_aid), "First Aid");
	else if (Q_stricmp(cmd, "level_om") == 0)
		Cmd_Level_Skill(ent, &(ent->client->pers.outdoorsman), "Outdoorsman");
	else if (Q_stricmp(cmd, "bloody_mess") == 0)
		Cmd_Unlock_Perk(ent, &(ent->client->pers.bloody_mess), "Bloody Mess");
	else if (Q_stricmp(cmd, "begin_special") == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "You have six SPECIAL stats:\nStrength, Perception, Endurance, Intelligence, Agility, & Luck\nBefore you begin, you must assign your points.\n");
		gi.cprintf(ent, PRINT_HIGH, "You have %d SPECIAL points to assign.\n", ent->client->pers.sp);
		gi.cprintf(ent, PRINT_HIGH, "You can use the command set_special to set all six stats at once or you can set each stat individually using the associated command, like set_str or set_agl.\n");
	}
	else if (Q_stricmp(cmd, "level_up") == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "You have six skills to level up:\nSmall Guns, Big Guns, Energy Weapons, Traps, First Aid, & Outdoorsman\nTo level up skills, you spend skill points\n");
		gi.cprintf(ent, PRINT_HIGH, "You have %d skill points to allocate.\n", ent->client->pers.sp);
		gi.cprintf(ent, PRINT_HIGH, "You can level up each skill using the associated command, like level_sg or level_om.\n");
	}
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
