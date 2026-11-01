#include <TiltedOnlinePCH.h>

#include <Games/ActorExtension.h>

bool ActorExtension::IsRemote() const noexcept
{
    return onlineFlags & kRemote;
}

bool ActorExtension::IsLocal() const noexcept
{
    return !IsRemote();
}

bool ActorExtension::IsPlayer() const noexcept
{
    return onlineFlags & kPlayer;
}

bool ActorExtension::IsRemotePlayer() const noexcept
{
    return IsRemote() && IsPlayer();
}

bool ActorExtension::IsLocalPlayer() const noexcept
{
    return IsLocal() && IsPlayer();
}

void ActorExtension::SetRemote(bool aSet) noexcept
{
    if (aSet)
        onlineFlags |= kRemote;
    else
        onlineFlags &= ~kRemote;
}

void ActorExtension::SetPlayer(bool aSet) noexcept
{
    if (aSet)
        onlineFlags |= kPlayer;
    else
        onlineFlags &= ~kPlayer;
}

// SkyrimNet bridge: blacklist bit, kept equal to IsRemote() by SkyrimNetBridge::Reconcile (D8).
bool ActorExtension::IsSkyrimNetBlacklisted() const noexcept // SkyrimNet bridge:
{                                                             // SkyrimNet bridge:
    return onlineFlags & kSkyrimNetBlacklisted;               // SkyrimNet bridge:
}                                                             // SkyrimNet bridge:

void ActorExtension::SetSkyrimNetBlacklisted(bool aSet) noexcept // SkyrimNet bridge:
{                                                                // SkyrimNet bridge:
    if (aSet)                                                    // SkyrimNet bridge:
        onlineFlags |= kSkyrimNetBlacklisted;                    // SkyrimNet bridge:
    else                                                         // SkyrimNet bridge:
        onlineFlags &= ~kSkyrimNetBlacklisted;                   // SkyrimNet bridge:
}                                                                // SkyrimNet bridge:
