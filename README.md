# Unreal Crowd Control

This plugin uses the generic crowd control to connect via websockets to trigger events from a streaming platform (e.g. Twitch, Youtube)
to your game. You do not need to implement these websockets yourself.
For Websocket API: https://staging-developer.crowdcontrol.live/sockets/

You will need to make your own Game Pack for Crowd Control servers or else you will have to use ones from other games or samples.
Game Pack: https://staging-developer.crowdcontrol.live/sockets/pack.html

# Essential Assets
- *UCrowdControlSubsystem*: Core component that handles connecting to Crowd Control and setups up bindings to specific events.

# Example Content
- *BP_CrowdControlExampleCharacter*: Example character with minimal implementation. Just to demonstrate event bindings in Blueprint.
- *W_Example_Login*: Example UI to use CrowdControlSubsystem for login and connecting to a specific platform.
- *GM_CrowdControlExampleGamemode*: Only purpose is to make sure example assets are used.
