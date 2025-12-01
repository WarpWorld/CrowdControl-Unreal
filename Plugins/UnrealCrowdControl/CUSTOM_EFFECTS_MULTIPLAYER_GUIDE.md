# Custom Effects API - Multiplayer Usage Guide

This guide explains how to use the Custom Effects API in your multiplayer Unreal game to create dynamic, player-specific effects.

## Overview

The Custom Effects API allows you to:
- **Upload** custom effects that are specific to your game session (e.g., player-specific effects)
- **Clear** all custom effects (useful when starting a new session)
- **Delete** specific custom effects (useful when players leave)
- **Get** the current list of custom effects

## Basic Usage Pattern

### 1. When a Player Joins

Create player-specific effects and upload them:

```cpp
void AMyGameMode::OnPlayerJoin(APlayerController* NewPlayer, FString UniqueID)
{
    UCrowdControlSubsystem* CC = GetGameInstance()->GetSubsystem<UCrowdControlSubsystem>();
    
    // Check if Crowd Control is ready (connected and authenticated)
    if (!CC || !CC->IsReadyForCustomEffects())
    {
        UE_LOG(LogTemp, Warning, TEXT("Crowd Control not ready. Cannot create player effects. Connected: %d, Initialized: %d"), 
            CC ? CC->IsConnected() : false, 
            CC ? CC->IsInitialized() : false);
        return;
    }
    
    // Create player-specific effects
    FCrowdControlEffectInfo KillEffect;
    KillEffect.id = "kill_" + UniqueID;
    KillEffect.displayName = "Kill " + NewPlayer->GetPlayerName();
    KillEffect.description = "Instantly kills " + NewPlayer->GetPlayerName();
    KillEffect.price = 100;
    KillEffect.maxRetries = 3;
    KillEffect.retryDelay = 5.0f;
    KillEffect.pendingDelay = 0.0f;
    KillEffect.sellable = true;
    KillEffect.visible = true;
    KillEffect.nonPoolable = false;
    
    // Add categories for organization
    KillEffect.categories.Add(NewPlayer->GetPlayerName() + "Effects");
    KillEffect.categories.Add("PlayerActions");
    
    // Register the effect locally
    CC->SetupEffect(KillEffect);
    
    // Upload to Crowd Control server
    CC->UploadCustomEffects();
    
    // Bind to the effect trigger delegate
    CC->OnEffectTrigger.AddDynamic(this, &AMyGameMode::HandleCrowdControlEffect);
}

void AMyGameMode::HandleCrowdControlEffect(FString EffectID, FString DisplayName)
{
    // Check if this is a player-specific kill effect
    if (EffectID.StartsWith("kill_"))
    {
        FString PlayerID = EffectID.RightChop(5); // Remove "kill_" prefix
        // Find and kill the player
        // ... your implementation here
    }
}
```

### 2. When a Player Leaves

Remove their custom effects:

```cpp
void AMyGameMode::OnPlayerLeave(FString UniqueID)
{
    UCrowdControlSubsystem* CC = GetGameInstance()->GetSubsystem<UCrowdControlSubsystem>();
    
    if (!CC || !CC->IsInitialized())
    {
        return;
    }
    
    // Remove from local registry (optional - for cleanup)
    // Note: RemoveCustomEffect only removes from local GameJsonObject
    // You still need to delete from server
    
    // Delete from Crowd Control server
    TArray<FString> EffectsToDelete;
    EffectsToDelete.Add("kill_" + UniqueID);
    // Add any other player-specific effects
    // EffectsToDelete.Add("teleport_" + UniqueID);
    // EffectsToDelete.Add("freeze_" + UniqueID);
    
    CC->DeleteCustomEffects(EffectsToDelete);
}
```

### 3. Starting a New Game Session

Clear all custom effects when starting a new match:

```cpp
void AMyGameMode::StartNewMatch()
{
    UCrowdControlSubsystem* CC = GetGameInstance()->GetSubsystem<UCrowdControlSubsystem>();
    
    if (CC && CC->IsInitialized())
    {
        // Clear all custom effects from the server
        CC->ClearCustomEffects();
        
        // Optionally clear local registry
        // Note: You may want to keep local effects and re-upload them
    }
}
```

## Advanced Patterns

### Pattern 1: Team-Based Effects

```cpp
void AMyGameMode::SetupTeamEffects(FString TeamName, TArray<FString> PlayerIDs)
{
    UCrowdControlSubsystem* CC = GetGameInstance()->GetSubsystem<UCrowdControlSubsystem>();
    
    FCrowdControlEffectInfo TeamKillEffect;
    TeamKillEffect.id = "teamkill_" + TeamName;
    TeamKillEffect.displayName = "Kill All " + TeamName + " Players";
    TeamKillEffect.description = "Instantly kills all players on " + TeamName + " team";
    TeamKillEffect.price = 500;
    TeamKillEffect.categories.Add("TeamActions");
    TeamKillEffect.categories.Add(TeamName);
    
    CC->SetupEffect(TeamKillEffect);
    CC->UploadCustomEffects();
}
```

### Pattern 2: Timed Effects for Players

```cpp
void AMyGameMode::AddPlayerSpeedBoost(APlayerController* Player, FString UniqueID)
{
    UCrowdControlSubsystem* CC = GetGameInstance()->GetSubsystem<UCrowdControlSubsystem>();
    
    FCrowdControlTimedEffectInfo SpeedBoostEffect;
    SpeedBoostEffect.id = "speedboost_" + UniqueID;
    SpeedBoostEffect.displayName = "Speed Boost " + Player->GetPlayerName();
    SpeedBoostEffect.description = "Doubles movement speed for 30 seconds";
    SpeedBoostEffect.duration = 30.0f;
    SpeedBoostEffect.price = 200;
    SpeedBoostEffect.categories.Add("Buffs");
    
    CC->SetupTimedEffect(SpeedBoostEffect);
    CC->UploadCustomEffects();
}
```

### Pattern 3: Parameter Effects

```cpp
void AMyGameMode::AddPlayerTeleportEffect(APlayerController* Player, FString UniqueID, TArray<FString> LocationNames)
{
    UCrowdControlSubsystem* CC = GetGameInstance()->GetSubsystem<UCrowdControlSubsystem>();
    
    FCrowdControlParameterEffectInfo TeleportEffect;
    TeleportEffect.id = "teleport_" + UniqueID;
    TeleportEffect.displayName = "Teleport " + Player->GetPlayerName();
    TeleportEffect.description = "Teleport " + Player->GetPlayerName() + " to a location";
    TeleportEffect.price = 300;
    
    // Add parameter with options
    FCrowdControlParameter LocationParam;
    LocationParam._id = "location";
    LocationParam._options = LocationNames; // e.g., {"Spawn", "Arena", "SafeZone"}
    
    TeleportEffect.parameters.Add(LocationParam);
    TeleportEffect.categories.Add("Movement");
    
    CC->SetupParameterEffect(TeleportEffect);
    CC->UploadCustomEffects();
}
```

## Connection Status

Crowd Control has two connection states:

- **`IsConnected()`**: Returns `true` when WebSocket is connected (CommandID >= 2)
  - This means the connection to Crowd Control server is established
  - But you may not be authenticated yet

- **`IsInitialized()`**: Returns `true` when authenticated and ready (CommandID > 2)
  - This means you're connected AND authenticated
  - Required for Custom Effects API to work

- **`IsReadyForCustomEffects()`**: Helper function that checks both
  - Returns `true` only if both connected AND authenticated
  - Use this before calling Custom Effects API functions

**CommandID States:**
- `0-1`: Not connected
- `2`: Connected but not authenticated (needs login)
- `3+`: Connected and authenticated (ready to use)

## Best Practices

1. **Always Check Connection Status**: Before using any Custom Effects API functions, check both connection and initialization:
   ```cpp
   // Option 1: Use the helper function (recommended)
   if (!CC->IsReadyForCustomEffects()) return;
   
   // Option 2: Check both explicitly
   if (!CC->IsConnected() || !CC->IsInitialized()) return;
   
   // Option 3: Just check initialization (will fail gracefully with better error messages)
   if (!CC->IsInitialized()) return;
   ```
   
   The Custom Effects API functions now check both states internally and provide helpful error messages if either is false.

2. **Use Unique IDs**: Always include a unique identifier (player ID, session ID, etc.) in your effect IDs to avoid conflicts:
   ```cpp
   KillEffect.id = "kill_" + UniqueID;  // Good
   KillEffect.id = "kill";              // Bad - conflicts with other players
   ```

3. **Clean Up on Player Leave**: Always delete player-specific effects when they leave to keep the menu clean.

4. **Batch Operations**: When deleting multiple effects, use `DeleteCustomEffects()` with an array instead of calling `RemoveCustomEffect()` multiple times:
   ```cpp
   TArray<FString> EffectsToDelete;
   EffectsToDelete.Add("effect1_" + PlayerID);
   EffectsToDelete.Add("effect2_" + PlayerID);
   CC->DeleteCustomEffects(EffectsToDelete);  // One API call
   ```

5. **Monitor Authentication**: Use the `OnCommandIDChanged` delegate to know when authentication completes:
   ```cpp
   CC->OnCommandIDChanged.AddDynamic(this, &AMyGameMode::OnCrowdControlStatusChanged);
   
   void AMyGameMode::OnCrowdControlStatusChanged(int32 NewCommandID)
   {
       if (NewCommandID > 2) // Initialized
       {
           // Custom effects are automatically cleared here
           // Now you can set up your effects
           SetupGameEffects();
       }
   }
   ```

## API Reference

### `UploadCustomEffects()`
Uploads all effects currently in `GameJsonObject` to the Crowd Control server. Only uploads if `GameJsonObject.IsValid()` is true.

### `ClearCustomEffects()`
Removes all custom effects from the Crowd Control server. Called automatically on authentication.

### `DeleteCustomEffects(const TArray<FString>& EffectIDs)`
Deletes specific effects by their IDs. Use this when players leave.

### `RemoveCustomEffect(FString EffectID)`
Convenience wrapper that deletes a single effect. Internally calls `DeleteCustomEffects()`.

### `GetCustomEffects()`
Returns a JSON string of all current custom effects. Useful for debugging or syncing.

## Troubleshooting

**Effects not appearing?**
- Ensure `IsInitialized()` returns true
- Check that `UploadCustomEffects()` was called after `SetupEffect()`
- Verify effect IDs are unique and don't conflict with built-in effects

**Effects from previous session still showing?**
- This shouldn't happen - effects are automatically cleared on authentication
- If it does, manually call `ClearCustomEffects()` when starting a new session

**Player effects not being removed?**
- Ensure `DeleteCustomEffects()` is called with the correct effect IDs
- Check that the effect IDs match exactly (case-sensitive)

