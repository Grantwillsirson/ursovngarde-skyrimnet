#pragma once

// SkyrimNet bridge: compatibility service for SkyrimNet (AI NPC dialogue) in co-op.
// See docs/bridge/PLAN.md (Tier 1). Every bridge code path checks IsActive() first, so nothing
// changes when SkyrimNet.esp is not loaded.

struct World;
struct UpdateEvent;
struct Actor;
struct TESFaction;
struct ActorExtension;
struct Factions;

/**
 * @brief Detects SkyrimNet and keeps its AI on the game that owns each actor.
 *
 * T1.1 detects SkyrimNet.esp and resolves SkyrimNet_ActorBlacklistFaction.
 * T1.2-T1.5 (DECISIONS.md D8): Reconcile() runs from HookActorProcess for every actor every frame and
 * keeps "in SkyrimNet_ActorBlacklistFaction" equal to "remote in this game" (NPC copies and the
 * remote player avatar). Actor::SetFactions reports its faction wipe through OnFactionsReset(), and
 * Actor::GetFactions strips the faction through StripBlacklist() so it is never sent to another client.
 */
struct SkyrimNetBridge
{
    SkyrimNetBridge(World& aWorld, entt::dispatcher& aDispatcher);
    ~SkyrimNetBridge() noexcept = default;

    TP_NOCOPYMOVE(SkyrimNetBridge);

    //! SkyrimNet_ActorBlacklistFaction's record id in SkyrimNet.esp.
    static constexpr uint32_t kBlacklistFactionRecord = 0x0012DB;

    //! True only when SkyrimNet.esp is loaded as a standard plugin. Resolves once, after the plugin
    //! list is complete; false until then. Cheap, safe to call every frame.
    [[nodiscard]] bool IsActive() const noexcept;

    //! SkyrimNet_ActorBlacklistFaction, or nullptr when inactive or the form does not resolve.
    [[nodiscard]] TESFaction* GetBlacklistFaction() const noexcept;

    //! The blacklist faction's form id in this game's load order, 0 when inactive.
    [[nodiscard]] uint32_t GetBlacklistFactionId() const noexcept;

    //! T1.2-T1.5: per-frame reconciliation, called from HookActorProcess on the game thread for every
    //! actor we extend. Blacklists remote actors, un-blacklists local ones. Allocation-free; a flag
    //! compare when nothing changed.
    void Reconcile(Actor* apActor, ActorExtension* apExtension) noexcept;

    //! T1.4: Actor::SetFactions wiped every faction; the next Reconcile() re-adds the blacklist.
    void OnFactionsReset(ActorExtension* apExtension) noexcept;

    //! T1.4: removes the blacklist faction from an outgoing faction snapshot (Actor::GetFactions).
    void StripBlacklist(Factions& aFactions) const noexcept;

    //! True when the actor is currently a member of the blacklist faction, from its base NPC factions and
    //! ExtraFactionChanges (an entry with rank < 0 is a removal). Allocation-free; walks the same data as
    //! Actor::GetFactions. Used once per actor to pick up membership persisted in a save.
    [[nodiscard]] bool HasBlacklistFaction(const Actor* apActor) const noexcept;

    //! T1.2/T1.3/T1.5: an actor was put in the blacklist (it is remote in this game). Called by Reconcile().
    void OnActorBecameRemote(Actor* apActor) noexcept;
    //! T1.3: an actor was taken out of the blacklist (this game owns it). Called by Reconcile().
    void OnActorBecameLocal(Actor* apActor) noexcept;
    //! T1.5: the other player's avatar was added to this game. Stub.
    void OnRemotePlayerAdded(Actor* apActor) noexcept;

protected:
    void OnUpdate(const UpdateEvent& acEvent) noexcept;

private:
    enum class State : uint8_t
    {
        Unresolved,
        Active,
        Inactive
    };

    World& m_world;
    entt::dispatcher& m_dispatcher;
    entt::scoped_connection m_updateConnection;

    mutable State m_state{State::Unresolved};
    mutable uint32_t m_blacklistFactionId{0};
    mutable TESFaction* m_pBlacklistFaction{nullptr};
    mutable bool m_factionLooked{false};
    mutable bool m_stripFailureLogged{false};
};
