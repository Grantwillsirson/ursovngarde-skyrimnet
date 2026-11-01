#pragma once

// SkyrimNet bridge: compatibility service for SkyrimNet (AI NPC dialogue) in co-op.
// See docs/bridge/PLAN.md (Tier 1). Every bridge code path checks IsActive() first, so nothing
// changes when SkyrimNet.esp is not loaded.

struct World;
struct UpdateEvent;
struct Actor;
struct TESFaction;

/**
 * @brief Detects SkyrimNet and keeps its AI on the game that owns each actor.
 *
 * T1.1 (this file) only detects SkyrimNet.esp and resolves SkyrimNet_ActorBlacklistFaction.
 * The OnActor* hooks are stubs: T1.2 (remote copies), T1.3 (ownership transfer),
 * T1.4 (faction sync) and T1.5 (remote player avatar) fill them.
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

    //! T1.2/T1.3: an actor became a remote copy in this game. Stub.
    void OnActorBecameRemote(Actor* apActor) noexcept;
    //! T1.3: this game became the owner of an actor. Stub.
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
};
