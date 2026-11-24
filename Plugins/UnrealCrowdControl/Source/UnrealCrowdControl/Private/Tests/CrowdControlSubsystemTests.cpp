#if WITH_DEV_AUTOMATION_TESTS

#include "CrowdControlAutomationTestTypes.h"
#include "CrowdControlManifest.h"
#include "CrowdControlSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrowdControlManifestEnvelopeTest,
	"CrowdControl.Manifest.RequiredEnvelope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdControlManifestEnvelopeTest::RunTest(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Effects = MakeShared<FJsonObject>();
	Effects->SetStringField(TEXT("name"), TEXT("Jump"));
	TSharedPtr<FJsonObject> Events = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> Event = MakeShared<FJsonObject>();
	Event->SetStringField(TEXT("name"), TEXT("Player Jump"));
	Events->SetObjectField(TEXT("player_jump"), Event);

	const TSharedPtr<FJsonObject> Root = CrowdControlManifest::CreateRoot(Effects, Events);
	FString Serialized;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
	if (!TestTrue(TEXT("manifest serialization succeeds"), FJsonSerializer::Serialize(Root.ToSharedRef(), Writer)))
	{
		return false;
	}

	TSharedPtr<FJsonObject> Parsed;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Serialized);
	if (!TestTrue(TEXT("serialized manifest parses"), FJsonSerializer::Deserialize(Reader, Parsed)) || !Parsed.IsValid())
	{
		return false;
	}

	FString Kind;
	double Format = 0.0;
	TestTrue(TEXT("kind is a string"), Parsed->TryGetStringField(TEXT("kind"), Kind));
	TestEqual(TEXT("kind is built-in"), Kind, FString(TEXT("built-in")));
	TestTrue(TEXT("format is numeric"), Parsed->TryGetNumberField(TEXT("format"), Format));
	TestEqual(TEXT("format is version one"), Format, 1.0);
	const TSharedPtr<FJsonObject>* EffectsObject = nullptr;
	TestTrue(TEXT("effects remain at the top-level effects.game path"), Parsed->TryGetObjectField(TEXT("effects"), EffectsObject) && EffectsObject && EffectsObject->IsValid());
	const TSharedPtr<FJsonObject>* GameEffects = nullptr;
	TestTrue(TEXT("game effect metadata remains present"), EffectsObject && EffectsObject->IsValid()
		&& (*EffectsObject)->TryGetObjectField(TEXT("game"), GameEffects) && GameEffects && GameEffects->IsValid());
	FString EffectName;
	TestTrue(TEXT("game effect metadata is preserved"), GameEffects && GameEffects->IsValid()
		&& (*GameEffects)->TryGetStringField(TEXT("name"), EffectName));
	TestEqual(TEXT("effect name remains unchanged"), EffectName, FString(TEXT("Jump")));
	const TSharedPtr<FJsonObject>* MetaObject = nullptr;
	TestTrue(TEXT("events remain under meta.events"), Parsed->TryGetObjectField(TEXT("meta"), MetaObject) && MetaObject && MetaObject->IsValid());
	const TSharedPtr<FJsonObject>* EventMetadata = nullptr;
	TestTrue(TEXT("game event metadata remains present"), MetaObject && MetaObject->IsValid()
		&& (*MetaObject)->TryGetObjectField(TEXT("events"), EventMetadata) && EventMetadata && EventMetadata->IsValid());
	const TSharedPtr<FJsonObject>* PlayerJumpEvent = nullptr;
	TestTrue(TEXT("event definition remains unchanged"), EventMetadata && EventMetadata->IsValid()
		&& (*EventMetadata)->TryGetObjectField(TEXT("player_jump"), PlayerJumpEvent) && PlayerJumpEvent && PlayerJumpEvent->IsValid());
	FString EventName;
	TestTrue(TEXT("event name remains unchanged"), PlayerJumpEvent && PlayerJumpEvent->IsValid()
		&& (*PlayerJumpEvent)->TryGetStringField(TEXT("name"), EventName));
	TestEqual(TEXT("event name is preserved"), EventName, FString(TEXT("Player Jump")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrowdControlAuthListenerDetectionTest,
	"CrowdControl.AuthFallback.InvokableListeners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdControlAuthListenerDetectionTest::RunTest(const FString& Parameters)
{
	FOnAuthCodeReceived Delegate;
	TestFalse(TEXT("an empty delegate has no listener"), CrowdControlManifest::HasInvokableListeners(Delegate));
	TestTrue(TEXT("an empty delegate allows fallback"), CrowdControlManifest::ShouldOpenAuthURL(Delegate, true));
	TestFalse(TEXT("explicit opt-out disables fallback"), CrowdControlManifest::ShouldOpenAuthURL(Delegate, false));

	Delegate.Add(FOnAuthCodeReceived::FDelegate());
	TestFalse(TEXT("an unbound invocation entry is not a listener"), CrowdControlManifest::HasInvokableListeners(Delegate));
	TestTrue(TEXT("an unbound invocation entry still allows fallback"), CrowdControlManifest::ShouldOpenAuthURL(Delegate, true));

	UCrowdControlAuthDelegateTestObject* Listener = NewObject<UCrowdControlAuthDelegateTestObject>();
	Delegate.AddDynamic(Listener, &UCrowdControlAuthDelegateTestObject::ReceiveAuthCode);
	TestTrue(TEXT("a bound native UFunction is a listener"), CrowdControlManifest::HasInvokableListeners(Delegate));
	TestFalse(TEXT("a bound listener suppresses fallback"), CrowdControlManifest::ShouldOpenAuthURL(Delegate, true));

	Delegate.RemoveDynamic(Listener, &UCrowdControlAuthDelegateTestObject::ReceiveAuthCode);
	TestFalse(TEXT("removing the final listener restores fallback"), CrowdControlManifest::HasInvokableListeners(Delegate));
	TestTrue(TEXT("fallback returns after the final listener is removed"), CrowdControlManifest::ShouldOpenAuthURL(Delegate, true));

	Delegate.AddUniqueDynamic(Listener, &UCrowdControlAuthDelegateTestObject::ReceiveAuthCode);
	TestTrue(TEXT("a Blueprint-style unique dynamic binding is counted"), CrowdControlManifest::HasInvokableListeners(Delegate));
	Delegate.RemoveDynamic(Listener, &UCrowdControlAuthDelegateTestObject::ReceiveAuthCode);
	TestFalse(TEXT("removing the dynamic Blueprint-style binding restores fallback"), CrowdControlManifest::HasInvokableListeners(Delegate));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrowdControlMenuExportDiagnosticsTest,
	"CrowdControl.Manifest.ActionableExportFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdControlMenuExportDiagnosticsTest::RunTest(const FString& Parameters)
{
	TMap<FString, TPair<FString, FString>> Errors;
	FString Stage;
	FString Detail;
	TestFalse(TEXT("an empty error collection has no generation failure"),
		CrowdControlManifest::GetGenerationFailure(Errors, Stage, Detail));

	Errors.Add(
		TEXT("effect:broken"),
		TPair<FString, FString>(
			TEXT("metadata construction"),
			TEXT("Effect registration failed (effectID='broken', component='CrowdControlEffectComponent', owner='TestActor'): duplicate parameter ID 'mode'.")));
	TestTrue(TEXT("a generation failure is returned"), CrowdControlManifest::GetGenerationFailure(Errors, Stage, Detail));
	TestEqual(TEXT("the failing stage is retained"), Stage, FString(TEXT("metadata construction")));
	TestTrue(TEXT("actionable component and duplicate-ID detail is retained"),
		Detail.Contains(TEXT("effectID='broken', component='CrowdControlEffectComponent', owner='TestActor'): duplicate parameter ID 'mode'.")));

	Errors.Add(
		TEXT("event:broken"),
		TPair<FString, FString>(TEXT("metadata construction"), TEXT("Game event registration failed.")));
	TestTrue(TEXT("multiple generation failures are summarized"), CrowdControlManifest::GetGenerationFailure(Errors, Stage, Detail));
	TestTrue(TEXT("the summary points to additional diagnostics in the output log"),
		Detail.Contains(TEXT("and 1 additional generation error(s); see Output Log")));
	return true;
}

#endif
