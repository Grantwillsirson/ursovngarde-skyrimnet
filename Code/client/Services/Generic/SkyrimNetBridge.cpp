// SkyrimNet bridge: implementation of the SkyrimNet compatibility service. See docs/bridge/PLAN.md (Tier 1).

#include <Services/SkyrimNetBridge.h>

#include <World.h>

#include <Events/UpdateEvent.h>

#include <Actor.h>
#include <Forms/TESForm.h>
#include <Forms/TESFaction.h>
#include <Forms/TESNPC.h>
#include <ExtraData/ExtraFactionChanges.h>
#include <Games/TES.h>
#include <Games/ActorExtension.h>
#include <Structs/Factions.h>
#include <Systems/ModSystem.h>

SkyrimNetBridge::SkyrimNetBridge(World& aWorld, entt::dispatcher& aDispatcher)
    : m_world(aWorld)
    , m_dispatcher(aDispatcher)
{
    m_updateConnection = m_dispatcher.sink<UpdateEvent>().connect<&SkyrimNetBridge::OnUpdate>(this);
}

bool SkyrimNetBridge::IsActive() const noexcept
{
    if (m_state != State::Unresolved)
        return m_state == State::Active;

    // The plugin list is complete once the game data is loaded; until then, answer false and retry later.
    ModManager* pManager = ModManager::Get();
    if (!pManager || !pManager->GetByName("Skyrim.esm"))
        return false;

    const Mod* pMod = pManager->GetByName("SkyrimNet.esp");
    if (!pMod || !pMod->IsLoaded())
    {
        m_state = State::Inactive;
        spdlog::info("SkyrimNet bridge: inactive (SkyrimNet.esp not loaded)");
        return false;
    }

    // The faction's record id needs more than the 12 bits an ESL-flagged plugin has.
    if (pMod->IsLite())
    {
        m_state = State::Inactive;
        spdlog::warn("SkyrimNet bridge: SkyrimNet.esp is ESL-flagged; cannot compose the blacklist faction id; bridge inactive");
        return false;
    }

    m_blacklistFactionId = (static_cast<uint32_t>(pMod->standardId) << 24) | kBlacklistFactionRecord;
    m_state = State::Active;
    spdlog::info("SkyrimNet bridge: active (SkyrimNet.esp index 0x{:02X}, blacklist faction 0x{:08X})", static_cast<uint32_t>(pMod->standardId), m_blacklistFactionId);
    return true;
}

TESFaction* SkyrimNetBridge::GetBlacklistFaction() const noexcept
{
    if (!IsActive())
        return nullptr;

    if (!m_factionLooked)
    {
        m_factionLooked = true;
        m_pBlacklistFaction = Cast<TESFaction>(TESForm::GetById(m_blacklistFactionId));
        if (!m_pBlacklistFaction)
            spdlog::warn("SkyrimNet bridge: form 0x{:08X} is not a faction; SkyrimNet_ActorBlacklistFaction not found", m_blacklistFactionId);
    }

    return m_pBlacklistFaction;
}

uint32_t SkyrimNetBridge::GetBlacklistFactionId() const noexcept
{
    return IsActive() ? m_blacklistFactionId : 0;
}

void SkyrimNetBridge::Reconcile(Actor* apActor, ActorExtension* apExtension) noexcept
{
    // Hot path: runs for every extended actor every frame. Cached state compare first, then one bit compare.
    if (!apActor || !apExtension || !IsActive())
        return;

    // Copies of NPCs owned elsewhere (T1.2) and the other player's avatar (T1.5) are both remote.
    const bool cWantBlacklisted = apExtension->IsRemote();

    // Once per actor: a save made during co-op persists the faction on NPCs that were copies then. A local
    // actor's bit starts at 0, so read the real membership and let the compare below remove it.
    // Remote actors skip this: they are forced into the blacklist anyway (Checked is set below).
    if (!cWantBlacklisted && !apExtension->IsSkyrimNetChecked())
    {
        apExtension->SetSkyrimNetChecked(true);
        apExtension->SetSkyrimNetBlacklisted(HasBlacklistFaction(apActor));
    }

    if (cWantBlacklisted == apExtension->IsSkyrimNetBlacklisted())
        return;

    TESFaction* pFaction = GetBlacklistFaction();
    if (!pFaction)
        return;

    // Rank 0 adds the actor to the faction, rank -1 removes it (Papyrus AddToFaction / RemoveFromFaction).
    apActor->SetFactionRank(pFaction, cWantBlacklisted ? 0 : -1);
    apExtension->SetSkyrimNetBlacklisted(cWantBlacklisted);
    apExtension->SetSkyrimNetChecked(true);

    spdlog::debug("SkyrimNet bridge: actor {:X} {} SkyrimNet_ActorBlacklistFaction", apActor->formID, cWantBlacklisted ? "added to" : "removed from");

    if (cWantBlacklisted)
    {
        OnActorBecameRemote(apActor);
        if (apExtension->IsRemotePlayer())
            OnRemotePlayerAdded(apActor);
    }
    else
        OnActorBecameLocal(apActor);
}

void SkyrimNetBridge::OnFactionsReset(ActorExtension* apExtension) noexcept
{
    // RemoveFromAllFactions() also removed the blacklist; the next Reconcile() puts it back if still remote.
    if (apExtension)
        apExtension->SetSkyrimNetBlacklisted(false);
}

void SkyrimNetBridge::StripBlacklist(Factions& aFactions) const noexcept
{
    if (!IsActive())
        return;

    // Converted on every call rather than cached: the server mod ids are reassigned on each connection.
    // A plain array lookup for a standard plugin, and SkyrimNet.esp is never ESL-flagged here (see IsActive()).
    GameId blacklistId{0, 0};
    if (!m_world.GetModSystem().GetServerModId(m_blacklistFactionId, blacklistId))
    {
        if (!m_stripFailureLogged)
        {
            m_stripFailureLogged = true;
            spdlog::warn("SkyrimNet bridge: no server id for blacklist faction 0x{:08X}; outgoing factions are not stripped", m_blacklistFactionId);
        }
        return;
    }

    const auto cIsBlacklist = [&blacklistId](const Faction& acFaction) { return acFaction.Id == blacklistId; };
    std::erase_if(aFactions.NpcFactions, cIsBlacklist);
    std::erase_if(aFactions.ExtraFactions, cIsBlacklist);
}

bool SkyrimNetBridge::HasBlacklistFaction(const Actor* apActor) const noexcept
{
    if (!apActor || !IsActive())
        return false;

    bool hasFaction = false;

    if (const auto* pNpc = Cast<TESNPC>(apActor->baseForm))
    {
        const auto& factions = pNpc->actorData.factions;
        for (auto i = 0u; i < factions.length; ++i)
        {
            if (factions[i].faction && factions[i].faction->formID == m_blacklistFactionId)
            {
                hasFaction = true;
                break;
            }
        }
    }

    // ExtraFactionChanges overrides the base record; rank < 0 is what Papyrus RemoveFromFaction leaves behind.
    if (const auto* pChanges = Cast<ExtraFactionChanges>(apActor->extraData.GetByType(ExtraDataType::Faction)))
    {
        for (auto i = 0u; i < pChanges->entries.length; ++i)
        {
            const auto& entry = pChanges->entries[i];
            if (entry.faction && entry.faction->formID == m_blacklistFactionId)
                hasFaction = entry.rank >= 0;
        }
    }

    return hasFaction;
}

void SkyrimNetBridge::OnActorBecameRemote(Actor* apActor) noexcept
{
    if (!IsActive() || !apActor)
        return;

    spdlog::debug("SkyrimNet bridge: actor {:X} became remote; SkyrimNet AI suppressed here", apActor->formID);

    // TODO(T1.3): losing ownership should drop this actor's queued SkyrimNet lines. SkyrimNet's Papyrus API
    // has no per-actor purge (SkyrimNetApi.PurgeDialogue is global); a config-flagged global
    // PurgeDialogue(true) is deferred to a later task. Nothing is called into SkyrimNet here.
}

void SkyrimNetBridge::OnActorBecameLocal(Actor* apActor) noexcept
{
    if (!IsActive() || !apActor)
        return;

    spdlog::debug("SkyrimNet bridge: actor {:X} became local; SkyrimNet AI runs here", apActor->formID);
}

void SkyrimNetBridge::OnRemotePlayerAdded(Actor* apActor) noexcept
{
    if (!IsActive() || !apActor)
        return;

    spdlog::debug("SkyrimNet bridge: remote player {:X} blacklisted", apActor->formID);
}

void SkyrimNetBridge::OnUpdate(const UpdateEvent&) noexcept
{
    // Resolves (and logs) once the game data is loaded; afterwards a single compare per frame.
    (void)IsActive();
}
