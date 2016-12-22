/*
 * helpers.cpp
 *
 *  Created on: Oct 8, 2016
 *      Author: nullifiedcat
 */

#include "common.h"
#include "hooks.h"
#include "sdk.h"

#include <pwd.h>
#include <sys/mman.h>

FILE* hConVarsFile = 0;
void BeginConVars() {
	passwd* pwd = getpwuid(getuid());
	char* user = pwd->pw_name;
	hConVarsFile = fopen(strfmt("/home/%s/.local/share/Steam/steamapps/common/Team Fortress 2/tf/cfg/cat_defaults.cfg", user), "w");
	SetCVarInterface(interfaces::cvar);
}

void EndConVars() {
	if (hConVarsFile) fclose(hConVarsFile);
	ConVar_Register();
}


bool IsPlayerInvulnerable(CachedEntity* player) {
	int cond1 = CE_INT(player, netvar.iCond);
	int cond2 = CE_INT(player, netvar.iCond1);
	int uber_mask_1 = (cond::uber | cond::bonk);
	int uber_mask_2 = (cond_ex::hidden_uber | cond_ex::canteen_uber | cond_ex::misc_uber | cond_ex::phlog_uber);
	if ((cond1 & uber_mask_1) || (cond2 & uber_mask_2)) {
		//logging::Info("COND1: %i MASK1: %i", cond1, uber_mask_1);
		//logging::Info("COND2: %i MASK2: %i", cond2, uber_mask_2);
		return true;
	}
	return false;
}

bool IsPlayerCritBoosted(CachedEntity* player) {
	int cond1 = CE_INT(player, netvar.iCond);
	int cond2 = CE_INT(player, netvar.iCond1);
	int cond4 = CE_INT(player, netvar.iCond3);
	int crit_mask_1 = (cond::kritzkrieg);
	int crit_mask_2 = (cond_ex::halloween_crit | cond_ex::canteen_crit | cond_ex::first_blood_crit | cond_ex::winning_crit |
			cond_ex::intelligence_crit | cond_ex::on_kill_crit | cond_ex::phlog_crit | cond_ex::misc_crit);
	int crit_mask_4 = (cond_ex3::powerup_crit);
	if ((cond1 & crit_mask_1) || (cond2 & crit_mask_2) || (cond4 & crit_mask_4)) return true;
	return false;
}

ConVar* CreateConVar(const char* name, const char* value, const char* help) {
	ConVar* ret = new ConVar(name, value, 0, help);
	if (hConVarsFile)
		fprintf(hConVarsFile, "%s %s\n", name, value);
	interfaces::cvar->RegisterConCommand(ret);
	return ret;
}

ConCommand* CreateConCommand(const char* name, FnCommandCallback_t callback, const char* help) {
	ConCommand* ret = new ConCommand(name, callback, help);
	interfaces::cvar->RegisterConCommand(ret);
	return ret;
}

const char* GetModelPath(CachedEntity* entity) {
	if (!entity) return "NULL";
	const model_t* model = RAW_ENT(entity)->GetModel();
	return interfaces::model->GetModelName(model);
}

const char* GetBuildingName(CachedEntity* ent) {
	if (!ent) return "[NULL]";
	switch (ent->m_iClassID) {
	case ClassID::CObjectSentrygun:
		// TODO mini
		return "Sentry";
	case ClassID::CObjectDispenser:
		return "Dispenser";
	case ClassID::CObjectTeleporter:
		// TODO exit/entr
		return "Teleporter";
	}
	return "[NULL]";
}

/* Takes CBaseAnimating entity as input */
item_type GetItemType(CachedEntity* entity) {
	if (entity == 0) return item_type::item_null;
	const char* path = GetModelPath(entity); /* SDK function */
	size_t length = strlen(path);
	/* Default/Festive medkits */
	if (length >= 29 && path[16] == 'k') {
		if (path[20] == 's') return item_type::item_medkit_small;
		if (path[20] == 'm') return item_type::item_medkit_medium;
		if (path[20] == 'l') return item_type::item_medkit_large;
	}
	/* Sandwich/Steak */
	if (length >= 22 && path[13] == 'p' && path[14] == 'l') {
		return item_type::item_medkit_medium;
	}
	/* Medieval meat */
	if (length == 39 && path[31] == 'm' && path[29] == 'l') {
		return item_type::item_medkit_medium;
	}
	/* Halloween medkits */
	if (length >= 49 && path[33] == 'm' && path[36] == 'k') {
		if (path[20] == 's') return item_type::item_medkit_small;
		if (path[40] == 'm') return item_type::item_medkit_medium;
		if (path[40] == 'l') return item_type::item_medkit_large;
	}
	/* Ammo packs */
	if (length >= 31 && path[14] == 'm' && path[15] == 'm') {
		if (path[22] == 's') return item_type::item_ammo_small;
		if (path[22] == 'm') return item_type::item_ammo_medium;
		if (path[22] == 'l') return item_type::item_ammo_large;
	}
	/* Powerups */
	if (length >= 38 && path[20] == 'p' && path[24] == 'w') {
		if (path[30] == 'h') return item_type::item_mp_haste;
		if (path[30] == 'v') return item_type::item_mp_vampire;
		if (path[30] == 'u') return item_type::item_mp_uber;
		if (path[32] == 'e') return item_type::item_mp_precision;
		if (path[30] == 'w') return item_type::item_mp_warlock;
		if (path[32] == 'r') return item_type::item_mp_strength;
		if (path[32] == 'g') return item_type::item_mp_regeneration;
		if (path[37] == 'v') return item_type::item_mp_supernova;
		/* looks like resistance.mdl is unused and replaced with defense.mdl? idk */
		if (path[37] == 'n') return item_type::item_mp_resistance;
		if (path[34] == 'k') return item_type::item_mp_knockout;
		/* actually this one is 'defense' but it's used for resistance powerup */
		if (path[35] == 's') return item_type::item_mp_resistance;
		if (path[30] == 'c') return item_type::item_mp_crit;
		if (path[30] == 'a') return item_type::item_mp_agility;
		if (path[31] == 'i') return item_type::item_mp_king;
		if (path[33] == 'g') return item_type::item_mp_plague;
		if (path[36] == 't') return item_type::item_mp_reflect;
		if (path[30] == 't') return item_type::item_mp_thorns;
	}
	return item_type::item_null;
}

powerup_type GetPowerupOnPlayer(CachedEntity* player) {
	if (!player) return powerup_type::not_powerup;
	int cond2 = CE_INT(player, netvar.iCond2);
	int cond3 = CE_INT(player, netvar.iCond3);
	//if (!(cond2 & cond_ex2::powerup_generic)) return powerup_type::not_powerup;
	if (cond2 & cond_ex2::powerup_strength) return powerup_type::strength;
	if (cond2 & cond_ex2::powerup_haste) return powerup_type::haste;
	if (cond2 & cond_ex2::powerup_regen) return powerup_type::regeneration;
	if (cond2 & cond_ex2::powerup_resistance) return powerup_type::resistance;
	if (cond2 & cond_ex2::powerup_vampire) return powerup_type::vampire;
	if (cond2 & cond_ex2::powerup_reflect) return powerup_type::reflect;
	if (cond3 & cond_ex3::powerup_precision) return powerup_type::precision;
	if (cond3 & cond_ex3::powerup_agility) return powerup_type::agility;
	if (cond3 & cond_ex3::powerup_knockout) return powerup_type::knockout;
	if (cond3 & cond_ex3::powerup_king) return powerup_type::king;
	if (cond3 & cond_ex3::powerup_plague) return powerup_type::plague;
	if (cond3 & cond_ex3::powerup_supernova) return powerup_type::supernova;

	return powerup_type::not_powerup;
}

void VectorTransform (const float *in1, const matrix3x4_t& in2, float *out)
{
	out[0] = (in1[0] * in2[0][0] + in1[1] * in2[0][1] + in1[2] * in2[0][2]) + in2[0][3];
	out[1] = (in1[0] * in2[1][0] + in1[1] * in2[1][1] + in1[2] * in2[1][2]) + in2[1][3];
	out[2] = (in1[0] * in2[2][0] + in1[1] * in2[2][1] + in1[2] * in2[2][2]) + in2[2][3];
}

bool GetHitbox(CachedEntity* entity, int hb, Vector& out) {
	if (CE_BAD(entity)) return false;
	const model_t* model = RAW_ENT(entity)->GetModel();
	if (!model) return false;
	studiohdr_t* shdr = interfaces::model->GetStudiomodel(model);
	if (!shdr) return false;
	mstudiohitboxset_t* set = shdr->pHitboxSet(CE_INT(entity, netvar.iHitboxSet));
	if (!set) return false;
	mstudiobbox_t* box = set->pHitbox(hb);
	if (!box) return false;
	if (box->bone < 0 || box->bone >= 128) return 5;
	//float *min = new float[3],
	//	  *max = new float[3];
	Vector min, max;
	SEGV_BEGIN
	if (entity->GetBones() == 0) logging::Info("no bones!");
	VectorTransform(box->bbmin, entity->GetBones()[box->bone], min);
	VectorTransform(box->bbmax, entity->GetBones()[box->bone], max);
	SEGV_END_INFO("VectorTransform()-ing with unsafe Vector casting");
	out.x = (min[0] + max[0]) / 2;
	out.x = (min[1] + max[1]) / 2;
	out.x = (min[2] + max[2]) / 2;
	//delete[] min;
	//delete[] max;
	return true;
}

void VectorAngles(Vector &forward, Vector &angles) {
	float tmp, yaw, pitch;

	if(forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if(forward[2] > 0)
			pitch = 270;
		else
			pitch = 90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / PI);
		if(yaw < 0)
			yaw += 360;

		tmp = sqrt((forward[0] * forward[0] + forward[1] * forward[1]));
		pitch = (atan2(-forward[2], tmp) * 180 / PI);
		if(pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

void FixMovement(CUserCmd& cmd, Vector& viewangles) {
	Vector movement(cmd.forwardmove, cmd.sidemove, cmd.upmove);
	float speed = sqrt(movement.x * movement.x + movement.y * movement.y);
	Vector ang;
	VectorAngles(movement, ang);
	float yaw = DEG2RAD(ang.y - viewangles.y + cmd.viewangles.y);
	cmd.forwardmove = cos(yaw) * speed;
	cmd.sidemove = sin(yaw) * speed;
	/*Vector vsilent(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	float speed = sqrt(vsilent.x * vsilent.x + vsilent.y * vsilent.y);
	Vector ang;
	VectorAngles(vsilent, ang);
	float yaw = deg2rad(ang.y - viewangles_old.y + cmd->viewangles.y);
	cmd->forwardmove = cos(yaw) * speed;
	cmd->sidemove = sin(yaw) * speed;*/
}

float deg2rad(float deg) {
	return deg * (PI / 180);
}

bool IsPlayerInvisible(CachedEntity* player) {
	int cond = CE_INT(player, netvar.iCond);
	int mask = cloaked;
	int cond_1 = CE_INT(player, netvar.iCond1);
	int mask_1 = cond_ex2::cloak_spell | cond_ex2::cloak_spell_fading;
	int mask_v = on_fire | jarate | milk;
	return !((cond & mask_v) || !((cond & mask) || (cond_1 & mask_1)));
}

float RandFloatRange(float min, float max)
{
    return (min + 1) + (((float) rand()) / (float) RAND_MAX) * (max - (min + 1));
}

trace::FilterDefault* trace_filter;
bool IsEntityVisible(CachedEntity* entity, int hb) {
	if (entity == g_pLocalPlayer->entity) return true;
	if (!trace_filter) {
		trace_filter = new trace::FilterDefault();
	}
	trace_t trace_visible;
	Ray_t ray;
	CachedEntity* local = ENTITY(interfaces::engineClient->GetLocalPlayer());
	trace_filter->SetSelf(RAW_ENT(local));
	Vector hit;
	if (hb == -1) {
		hit = entity->m_vecOrigin;
	} else {
		SAFE_CALL( \
				if (!GetHitbox(entity, hb, hit)) { \
					return false; \
		});
	}
	ray.Init(local->m_vecOrigin + g_pLocalPlayer->v_ViewOffset, hit);
	interfaces::trace->TraceRay(ray, 0x4200400B, trace_filter, &trace_visible);
	if (trace_visible.m_pEnt) {
		return ((IClientEntity*)trace_visible.m_pEnt) == RAW_ENT(entity);
	}
	return false;
}

Vector GetBuildingPosition(CachedEntity* ent) {
	Vector res = ent->m_vecOrigin;
	switch (ent->m_iClassID) {
	case ClassID::CObjectDispenser:
		res.z += 30;
		break;
	case ClassID::CObjectTeleporter:
		res.z += 8;
		break;
	case ClassID::CObjectSentrygun:
		switch (CE_INT(ent, netvar.iUpgradeLevel)) {
		case 1:
			res.z += 30;
			break;
		case 2:
			res.z += 50;
			break;
		case 3:
			res.z += 60;
			break;
		}
		break;
	}
	return res;
}

bool IsBuildingVisible(CachedEntity* ent) {
	if (!trace_filter) {
		trace_filter = new trace::FilterDefault();
	}
	trace_t trace_visible;
	Ray_t ray;
	trace_filter->SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	ray.Init(g_pLocalPlayer->v_Eye, GetBuildingPosition(ent));
	interfaces::trace->TraceRay(ray, 0x4200400B, trace_filter, &trace_visible);
	return (IClientEntity*)trace_visible.m_pEnt == RAW_ENT(ent);
}

void fVectorAngles(Vector &forward, Vector &angles) {
	float tmp, yaw, pitch;

	if(forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if(forward[2] > 0)
			pitch = 270;
		else
			pitch = 90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / PI);
		if(yaw < 0)
			yaw += 360;

		tmp = sqrt((forward[0] * forward[0] + forward[1] * forward[1]));
		pitch = (atan2(-forward[2], tmp) * 180 / PI);
		if(pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

void fClampAngle(Vector& qaAng) {
	while(qaAng[0] > 89)
		qaAng[0] -= 180;

	while(qaAng[0] < -89)
		qaAng[0] += 180;

	while(qaAng[1] > 180)
		qaAng[1] -= 360;

	while(qaAng[1] < -180)
		qaAng[1] += 360;

	qaAng.z = 0;
}

float DistToSqr(CachedEntity* entity) {
	if (CE_BAD(entity)) return 0.0f;
	return g_pLocalPlayer->v_Origin.DistToSqr(entity->m_vecOrigin);
}

bool IsMeleeWeapon(CachedEntity* ent) {
	switch (ent->m_iClassID) {
	case ClassID::CTFBat:
	case ClassID::CTFBat_Fish:
	case ClassID::CTFBat_Giftwrap:
	case ClassID::CTFBat_Wood:
	case ClassID::CTFShovel:
	case ClassID::CTFKatana:
	case ClassID::CTFFireAxe:
	case ClassID::CTFBottle:
	case ClassID::CTFSword:
	case ClassID::CTFFists:
	case ClassID::CTFWrench:
	case ClassID::CTFRobotArm:
	case ClassID::CTFBonesaw:
	case ClassID::CTFClub:
	case ClassID::CTFKnife:
		return true;
	}
	return false;
}

void Patch(void* address, void* patch, size_t length) {
	void* page = (void*)((uintptr_t)address &~ 0xFFF);
	mprotect(page, 0xFFF, PROT_WRITE | PROT_EXEC);
	memcpy(address, patch, length);
	mprotect(page, 0xFFF, PROT_EXEC);
}

bool IsProjectileCrit(CachedEntity* ent) {
	if (ent->m_bGrenadeProjectile)
		return CE_BYTE(ent, netvar.Grenade_bCritical);
	return CE_BYTE(ent, netvar.Rocket_bCritical);
}

weaponmode GetWeaponMode(CachedEntity* player) {
	int weapon_handle = CE_INT(player, netvar.hActiveWeapon);
	CachedEntity* weapon = (weapon_handle & 0xFFF < HIGHEST_ENTITY ? ENTITY(weapon_handle & 0xFFF) : 0);
	if (CE_BAD(weapon)) return weaponmode::weapon_invalid;
	if (IsMeleeWeapon(weapon)) return weaponmode::weapon_melee;
	switch (weapon->m_iClassID) {
	case ClassID::CTFLunchBox:
	case ClassID::CTFLunchBox_Drink:
	case ClassID::CTFBuffItem:
		return weaponmode::weapon_consumable;
	case ClassID::CTFRocketLauncher_DirectHit:
	case ClassID::CTFRocketLauncher:
	case ClassID::CTFGrenadeLauncher:
	case ClassID::CTFCompoundBow:
	case ClassID::CTFBat_Wood:
	case ClassID::CTFBat_Giftwrap:
	case ClassID::CTFFlareGun:
	case ClassID::CTFFlareGun_Revenge:
	case ClassID::CTFSyringeGun:
		return weaponmode::weapon_projectile;
	case ClassID::CTFJar:
	case ClassID::CTFJarMilk:
		return weaponmode::weapon_throwable;
	case ClassID::CTFWeaponPDA_Engineer_Build:
	case ClassID::CTFWeaponPDA_Engineer_Destroy:
	case ClassID::CTFWeaponPDA_Spy:
		return weaponmode::weapon_pda;
	case ClassID::CWeaponMedigun:
		return weaponmode::weapon_medigun;
	};
	return weaponmode::weapon_hitscan;
}

// TODO FIX this function
bool GetProjectileData(CachedEntity* weapon, float& speed, float& gravity) {
	if (!CE_BAD(weapon)) return false;
	float rspeed = 0.0f;
	float rgrav = 0.0f;
	typedef float(GetProjectileData)(IClientEntity*);
	switch (weapon->m_iClassID) {
	case ClassID::CTFRocketLauncher_DirectHit:
		rspeed = 1980.0f;
	break;
	case ClassID::CTFRocketLauncher:
		rspeed = 1100.0f;
	break;
	case ClassID::CTFGrenadeLauncher:
		// TODO offset (GetProjectileSpeed)
		rspeed = ((GetProjectileData*) *(*(const void ***) weapon + 527))(RAW_ENT(weapon));
		// TODO Wrong grenade launcher gravity
		rgrav = 0.5f;
	break;
	case ClassID::CTFCompoundBow: {
		rspeed = ((GetProjectileData*) *(*(const void ***) weapon + 527))(RAW_ENT(weapon));
		rgrav = ((GetProjectileData*) *(*(const void ***) weapon + 528))(RAW_ENT(weapon));
	} break;
	case ClassID::CTFBat_Wood:
		rspeed = 3000.0f;
		rgrav = 0.5f;
	break;
	case ClassID::CTFFlareGun:
		rspeed = 2000.0f;
		rgrav = 0.5f;
	break;
	case ClassID::CTFSyringeGun:
		rgrav = 0.2f;
		rspeed = 990.0f;
	break;
	default:
		return false;
	}
	speed = rspeed;
	gravity = rgrav;
	return true;
}

bool Developer(CachedEntity* ent) {
	return (ent->m_pPlayerInfo && ent->m_pPlayerInfo->friendsID == 347272825UL);
}

/*const char* MakeInfoString(IClientEntity* player) {
	char* buf = new char[256]();
	player_info_t info;
	if (!interfaces::engineClient->GetPlayerInfo(player->entindex(), &info)) return (const char*)0;
	logging::Info("a");
	int hWeapon = NET_INT(player, netvar.hActiveWeapon);
	if (NET_BYTE(player, netvar.iLifeState)) {
		sprintf(buf, "%s is dead %s", info.name, tfclasses[NET_INT(player, netvar.iClass)]);
		return buf;
	}
	if (hWeapon) {
		IClientEntity* weapon = ENTITY(hWeapon & 0xFFF);
		sprintf(buf, "%s is %s with %i health using %s", info.name, tfclasses[NET_INT(player, netvar.iClass)], NET_INT(player, netvar.iHealth), weapon->GetClientClass()->GetName());
	} else {
		sprintf(buf, "%s is %s with %i health", info.name, tfclasses[NET_INT(player, netvar.iClass)], NET_INT(player, netvar.iHealth));
	}
	logging::Info("Result: %s", buf);
	return buf;
}*/

trace::FilterNoPlayer* vec_filter;
bool IsVectorVisible(Vector origin, Vector target) {
	//logging::Info("ISVV");
	if (!vec_filter) {
		vec_filter = new trace::FilterNoPlayer();
	}
	trace_t trace_visible;
	Ray_t ray;
	vec_filter->SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	ray.Init(origin, target);
	interfaces::trace->TraceRay(ray, 0x4200400B, vec_filter, &trace_visible);
	float dist2 = origin.DistToSqr(trace_visible.endpos);
	float dist1 = origin.DistToSqr(target);
	//logging::Info("Target: %.1f, %.1f, %.1f; ENDPOS: %.1f, %.1f, %.1f", target.x, target.y, target.z, trace_visible.endpos.x, trace_visible.endpos.y, trace_visible.endpos.z);
	//logging::Info("Dist1: %f, Dist2: %f");
	return (dist1 <= dist2);
}

/*bool PredictProjectileAim(Vector origin, IClientEntity* target, hitbox_t hb, float speed, bool arc, float gravity, Vector& result) {
	if (!target) return false;
	//logging::Info("PRED PROJ AIM");
	//logging::Info("ProjSpeed: %f", speed);
	Vector res1 = result;
	if (GetHitboxPosition(target, hb, result)) {
		res1 = target->GetAbsOrigin();
	}
	int flags = NET_INT(target, eoffsets.iFlags);
	bool ground = (flags & (1 << 0));
	float distance = origin.DistTo(res1);
	float latency = interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) +
		interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);
	if (speed == 0) return false;
	float time = distance / speed + latency;
	if (!ground) {
		res1.z -= (400 * time * time);
	}
	res1 += NET_VECTOR(target, eoffsets.vVelocity) * time;
	if (arc)
		res1.z += (800 * 0.5 * gravity * time * time);
	result = res1;
	//if (!IsVisible();
	return IsVectorVisible(origin, res1);
}*/

relation GetRelation(CachedEntity* ent) {
	if (!ent->m_pPlayerInfo) return relation::NEUTRAL;
	for (int i = 0; i < n_friends; i++) {
		if (friends[i] == ent->m_pPlayerInfo->friendsID) return relation::FRIEND;
	}
	for (int i = 0; i < n_rage; i++) {
		if (rage[i] == ent->m_pPlayerInfo->friendsID) return relation::RAGE;
	}
	if (Developer(ent)) return relation::DEVELOPER;
	return relation::NEUTRAL;
}

bool IsSentryBuster(CachedEntity* entity) {
	return (entity->m_Type == EntityType::ENTITY_PLAYER &&
			CE_INT(entity, netvar.iClass) == tf_class::tf_demoman &&
			g_pPlayerResource->GetMaxHealth(entity) == 2500);
}

bool IsAmbassador(CachedEntity* entity) {
	if (entity->m_iClassID != ClassID::CTFRevolver) return false;
	int defidx = CE_INT(entity, netvar.iItemDefinitionIndex);
	return (defidx == 61 || defidx == 1006);
}

// F1 c&p
Vector CalcAngle(Vector src, Vector dst) {
	Vector AimAngles;
	Vector delta = src - dst;
	float hyp = sqrtf((delta.x * delta.x) + (delta.y * delta.y)); //SUPER SECRET IMPROVEMENT CODE NAME DONUT STEEL
	AimAngles.x = atanf(delta.z / hyp) * RADPI;
	AimAngles.y = atanf(delta.y / delta.x) * RADPI;
	AimAngles.z = 0.0f;
	if(delta.x >= 0.0)
		AimAngles.y += 180.0f;
	return AimAngles;
}

void MakeVector(Vector angle, Vector& vector)
{
	float pitch = float(angle[0] * PI / 180);
	float yaw = float(angle[1] * PI / 180);
	float tmp = float(cos(pitch));
	vector[0] = float(-tmp * -cos(yaw));
	vector[1] = float(sin(yaw)*tmp);
	vector[2] = float(-sin(pitch));
}

float GetFov(Vector angle, Vector src, Vector dst)
{
	Vector ang, aim;
	ang = CalcAngle(src, dst);

	MakeVector(angle, aim);
	MakeVector(ang, ang);

	float mag = sqrtf(pow(aim.x, 2) + pow(aim.y, 2) + pow(aim.z, 2));
	float u_dot_v = aim.Dot(ang);

	return RAD2DEG(acos(u_dot_v / (pow(mag, 2))));
}

bool CanHeadshot() {
	return (g_pLocalPlayer->flZoomBegin > 0.0f && (interfaces::gvars->curtime - g_pLocalPlayer->flZoomBegin > 0.2f));
}

bool CanShoot() {
	float tickbase = (float)(CE_INT(g_pLocalPlayer->entity, netvar.nTickBase)) * interfaces::gvars->interval_per_tick;
	float nextattack = CE_FLOAT(g_pLocalPlayer->weapon, netvar.flNextPrimaryAttack);
	return nextattack <= tickbase;
}

QAngle VectorToQAngle(Vector in) {
	return *(QAngle*)&in;
}

Vector QAngleToVector(QAngle in) {
	return *(Vector*)&in;
}

void AimAt(Vector origin, Vector target, CUserCmd* cmd) {
	Vector angles;
	Vector tr = (target - origin);
	fVectorAngles(tr, angles);
	fClampAngle(angles);
	cmd->viewangles = angles;
}

void AimAtHitbox(CachedEntity* ent, int hitbox, CUserCmd* cmd) {
	Vector r = ent->m_vecOrigin;
	GetHitbox(ent, hitbox, r);
	AimAt(g_pLocalPlayer->v_Eye, r, cmd);
}

bool IsEntityVisiblePenetration(CachedEntity* entity, int hb) {
	if (!trace::g_pFilterPenetration) {
		trace::g_pFilterPenetration = new trace::FilterPenetration();
	}
	trace_t trace_visible;
	Ray_t ray;
	trace::g_pFilterPenetration->SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	trace::g_pFilterPenetration->Reset();
	Vector hit;
	int ret = GetHitbox(entity, hb, hit);
	if (ret) {
		return false;
	}
	ray.Init(g_pLocalPlayer->v_Origin + g_pLocalPlayer->v_ViewOffset, hit);
	interfaces::trace->TraceRay(ray, 0x4200400B, trace::g_pFilterPenetration, &trace_visible);
	bool s = false;
	if (trace_visible.m_pEnt) {
		s = ((IClientEntity*)trace_visible.m_pEnt) == RAW_ENT(entity);
	}
	if (!s) return false;
	interfaces::trace->TraceRay(ray, 0x4200400B, trace::g_pFilterDefault, &trace_visible);
	if (trace_visible.m_pEnt) {
		IClientEntity* ent = (IClientEntity*)trace_visible.m_pEnt;
		if (ent) {
			if (ent->GetClientClass()->m_ClassID == ClassID::CTFPlayer) {
				if (ent == RAW_ENT(entity)) return false;
				if (trace_visible.hitbox >= 0) {
					return true;
				}
			}
		}
	}
	return false;
}


class CMoveData;

/*void RunEnginePrediction(IClientEntity* ent, CUserCmd *ucmd) {
	// we are going to require some helper functions for this to work
	// notably SetupMove, FinishMove and ProcessMovement


	// setup the types of the functions
	typedef void(*SetupMoveFn)(IClientEntity *, CUserCmd *, class IMoveHelper *, CMoveData *);
	typedef void(*FinishMoveFn)(IClientEntity *, CUserCmd*, CMoveData*);
	typedef void(*ProcessMovementFn)(IClientEntity *, CMoveData *);
	typedef void(*StartTrackPredictionErrorsFn)(IClientEntity *);
	typedef void(*FinishTrackPredictionErrorsFn)(IClientEntity *);

	// get the vtable
	void **predictionVtable = *(void ***)interfaces::prediction;
	logging::Info("predictionVtable 0x%08x", predictionVtable);
	// get the functions
	SetupMoveFn oSetupMove = (SetupMoveFn) predictionVtable[19];
	FinishMoveFn oFinishMove = (FinishMoveFn) predictionVtable[20];

	// get the vtable
	void **gameMovementVtable = *(void ***)interfaces::gamemovement;
	logging::Info("gameMovementVtable 0x%08x", gameMovementVtable);
	// get the functions
	ProcessMovementFn oProcessMovement = (ProcessMovementFn) gameMovementVtable[2];
	StartTrackPredictionErrorsFn oStartTrackPredictionErrors = (StartTrackPredictionErrorsFn) gameMovementVtable[3];
	FinishTrackPredictionErrorsFn oFinishTrackPredictionErrors = (FinishTrackPredictionErrorsFn) gameMovementVtable[4];

	// use this as movedata (should be big enough - otherwise the stack will die!)
	unsigned char moveData[2048];
	CMoveData *pMoveData = (CMoveData *)&(moveData[0]);
	logging::Info("pMoveData 0x%08x", pMoveData);

	// back up globals
	float frameTime = interfaces::gvars->frametime;
	float curTime = interfaces::gvars->curtime;

	CUserCmd defaultCmd;
	if(ucmd == NULL)
	{
		ucmd = &defaultCmd;
	}

	// set the current command
	NET_VAR(ent, 0x105C, void*) = ucmd;

	// set up the globals
	interfaces::gvars->curtime =  interfaces::gvars->interval_per_tick * NET_INT(ent, netvar.nTickBase);
	interfaces::gvars->frametime = interfaces::gvars->interval_per_tick;

	oStartTrackPredictionErrors(ent);

	logging::Info("StartTrackPredictionErrors(ent)");
	oSetupMove(ent, ucmd, NULL, pMoveData);
	logging::Info("oSetupMove");
	oProcessMovement(ent, pMoveData);
	logging::Info("oProcessMovement");
	oFinishMove(ent, ucmd, pMoveData);
	logging::Info("oFinishMove");

	oFinishTrackPredictionErrors(ent);
	logging::Info("oFinishTrackPredictionErrors");
	// reset the current command
	NET_VAR(ent, 0x105C, void *) = 0;

	// restore globals
	interfaces::gvars->frametime = frameTime;
	interfaces::gvars->curtime = curTime;

	return;
}

float oldCurtime;
float oldFrametime;

void StartPrediction(CUserCmd* cmd) {
	oldCurtime = interfaces::gvars->curtime;
	oldFrametime = interfaces::gvars->frametime;
	interfaces::gvars->curtime = NET_INT(g_pLocalPlayer->entity, netvar.nTickBase) * interfaces::gvars->interval_per_tick;
	interfaces::gvars->frametime = interfaces::gvars->interval_per_tick;
	//interfaces::gamemovement->
}

void EndPrediction() {
	interfaces::gvars->curtime = oldCurtime;
	interfaces::gvars->frametime = oldFrametime;
}*/

char* strfmt(const char* fmt, ...) {
	char* buf = new char[1024];
	va_list list;
	va_start(list, fmt);
	vsprintf(buf, fmt, list);
	va_end(list);
	return buf;
}

const char* powerups[] = {
	"STRENGTH",
	"RESISTANCE",
	"VAMPIRE",
	"REFLECT",
	"HASTE",
	"REGENERATION",
	"PRECISION",
	"AGILITY",
	"KNOCKOUT",
	"KING",
	"PLAGUE",
	"SUPERNOVA",
	"CRITS"
};

uint32 friends[256];
uint32 rage[256];

int n_friends = 0;
int n_rage = 0;

const char* tfclasses[] = {
	"[NULL]",
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer"
};

const char* packs[] = {
	"+",
	"++",
	"+++"
};