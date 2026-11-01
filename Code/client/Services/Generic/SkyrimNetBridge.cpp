// SkyrimNet bridge: implementation of the SkyrimNet compatibility service. See docs/bridge/PLAN.md (Tier 1).

#include <Services/SkyrimNetBridge.h>

#include <World.h>

#include <Events/UpdateEvent.h>

#include <Actor.h>
#include <Forms/TESForm.h>
#include <Forms/TESFaction.h>
#include <Games/TES.h>

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

void SkyrimNetBridge::OnActorBecameRemote(Actor* apActor) noexcept
{
    if (!IsActive() || !apActor)
        return;

    spdlog::debug("SkyrimNet bridge: actor {:X} became remote (stub, T1.2)", apActor->formID);
}

void SkyrimNetBridge::OnActorBecameLocal(Actor* apActor) noexcept
{
    if (!IsActive() || !apActor)
        return;

    spdlog::debug("SkyrimNet bridge: actor {:X} became local (stub, T1.3)", apActor->formID);
}

void SkyrimNetBridge::OnRemotePlayerAdded(Actor* apActor) noexcept
{
    if (!IsActive() || !apActor)
        return;

    spdlog::debug("SkyrimNet bridge: remote player {:X} added (stub, T1.5)", apActor->formID);
}

void SkyrimNetBridge::OnUpdate(const UpdateEvent&) noexcept
{
    // Resolves (and logs) once the game data is loaded; afterwards a single compare per frame.
    (void)IsActive();
}
