# Unreal Crowd Control

The official [Crowd Control](https://crowdcontrol.live) plugin for Unreal Engine. It lets viewers on Twitch, YouTube, TikTok, Discord, and more trigger effects in your game in real time. The plugin handles the WebSocket connection, authentication, and effect lifecycle for you - you just register effects and react when they trigger.

- Developer documentation: https://developer.crowdcontrol.live/unreal/
- WebSocket protocol reference (handled by the plugin): https://developer.crowdcontrol.live/sockets/

This branch supports Unreal Engine **5.0–5.4** (Win64). For UE 4.27 support, check out the other branches.

## Features

- **Application (appID) authentication** - set your `ApplicationID` in settings and viewers authorize with a short code/URL (`OnAuthCodeReceived`). Legacy Twitch/YouTube/Discord platform login is still available for now.
- **Effect components** - drop a `CrowdControlEffectComponent` on any actor, fill in the effect's metadata, and override `OnEffectTriggered` in Blueprint or C++. The component auto-registers, auto-responds (success / temporary failure / permanent failure / pending for async), and self-manages timed effects with pause/resume.
- **Global effect delegates** - alternatively bind `OnEffectTrigger`, `OnTimedEffectTrigger`, and `OnParameterEffectTrigger` on the subsystem and route effects yourself.
- **Effect notifications** - `OnEffectRequestReceived` fires for every incoming effect with the viewer's name, so you can show "Viewer sent Effect!" popups from Blueprint with one binding.
- **Connection events** - typed `OnConnectionStateChanged` (Connecting / Disconnected / WaitingForLogin / Connected) and `OnSessionReady`, no magic numbers needed.
- **Effect responses with messages** - `EffectSuccess`, `EffectFailureTemporary`, and `EffectFailurePermanent` with viewer-facing explanation messages.
- **Menu reports** - show/hide effects and mark them available/unavailable from game state (`SetEffectVisibility`, `SetEffectAvailability`).
- **Pack metadata** - report game state to Crowd Control with `SendPackMetadata`.
- **Session control** - sessions start automatically by default, or drive them manually with `StartGameSession` / `StopGameSession`.
- **Custom effects** - upload, list, and delete custom effects at runtime.

## Setup

1. Copy `Plugins/UnrealCrowdControl` into your project's `Plugins` folder and enable it in Edit → Plugins.
2. Configure the plugin in Project Settings → CrowdControlSettings (or `DefaultGame.ini`):

```ini
[/Script/UnrealCrowdControl.CrowdControlDeveloperSettings]
GamePackID=UnrealDemo
GameName=Unreal Demo
ApplicationID=your-app-id
PublicClientKey=your-public-client-key
bStartSessionAutomatically=True
```

`GamePackID`/`GameName` identify your effect pack (`UnrealDemo` is the shared testing pack). `ApplicationID` and `PublicClientKey` are issued by Crowd Control when you register your application; when set, connecting uses the auth-code flow instead of the platform login prompt.

3. Call `Connect` on the `CrowdControlSubsystem`, display the code/URL from `OnAuthCodeReceived`, and you're live once the viewer authorizes.

See the [tutorial](https://developer.crowdcontrol.live/unreal/tutorial/example) for a full walkthrough with the example project.

## Getting Your Game Pack ID and Application ID

Both are issued by the Crowd Control team - see [Submitting your Effect Pack](https://developer.crowdcontrol.live/unreal/tutorial/submit-effects) for the full process:

1. Build your effect menu and call `PrintEffectsToJsonFile` to generate `Saved/CCMenu.json`.
2. Join the [Warp World Discord](https://discord.com/invite/warpworld), pick "Community Developer", and post in #cc-developers. or email developer@crowdcontrol.live
3. Send over your `CCMenu.json` and project name. You'll receive your **GamePackID**, plus an **ApplicationID** and **PublicClientKey** for the auth-code login flow.

Until then, the shared `UnrealDemo` pack works for testing.

## Key Classes

- **UCrowdControlSubsystem** - game instance subsystem that owns the connection, effect registration, trigger dispatch, responses, reports, metadata, and session control.
- **UCrowdControlEffectComponent** - self-contained per-effect actor component (recommended way to implement effects).
- **UCrowdControlDeveloperSettings** - `GamePackID`, `GameName`, `ApplicationID`, `PublicClientKey`, and session auto-start configuration.

## Example Content

- **BP_CrowdControlExampleCharacter** - example character with minimal implementation, demonstrating event bindings in Blueprint.
- **W_Example_Login** - example UI using the subsystem to connect and log in.
- **GM_CrowdControlExampleGamemode** - game mode wiring up the example assets.

## Native DLL

The plugin wraps a native `CrowdControl.dll` (in `Plugins/UnrealCrowdControl/Binaries/Win64/`) that implements the Crowd Control PubSub WebSocket protocol. Its source lives in the `CrowdControlGenericPlugin` repository.
