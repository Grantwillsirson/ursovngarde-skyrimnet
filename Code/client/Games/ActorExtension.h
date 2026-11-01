#pragma once

#include <Structs/ActionEvent.h>

struct ActorExtension
{
    enum
    {
        kRemote = 1 << 0,
        kPlayer = 1 << 1,
        kSkyrimNetBlacklisted = 1 << 2, // SkyrimNet bridge: in SkyrimNet_ActorBlacklistFaction (D8)
        kSkyrimNetChecked = 1 << 3,     // SkyrimNet bridge: saved faction membership read once
    };

    enum class ReconciliationStage
    {
        None,
        WaitingForDisable,
        WaitingFor3D
    };

    bool IsRemote() const noexcept;
    bool IsLocal() const noexcept;
    bool IsPlayer() const noexcept;
    bool IsRemotePlayer() const noexcept;
    bool IsLocalPlayer() const noexcept;
    void SetRemote(bool aSet) noexcept;
    void SetPlayer(bool aSet) noexcept;
    bool IsSkyrimNetBlacklisted() const noexcept;       // SkyrimNet bridge:
    void SetSkyrimNetBlacklisted(bool aSet) noexcept;   // SkyrimNet bridge:
    bool IsSkyrimNetChecked() const noexcept;           // SkyrimNet bridge:
    void SetSkyrimNetChecked(bool aSet) noexcept;       // SkyrimNet bridge:

    ActionEvent LatestAnimation{};
    size_t GraphDescriptorHash = 0;
    // The behavior project GraphDescriptorHash was taken from; the hash is taken again when it changes.
    uint64_t GraphDescriptorProjectKey = 0;

    // TODO: atomic? bool instead? maybe simplify to `IsReenabling()` ?
    // Protects discovery while rebuilding a leveled NPC.
    ReconciliationStage Reconciliation{ReconciliationStage::None};

private:
    uint32_t onlineFlags{0};
};
