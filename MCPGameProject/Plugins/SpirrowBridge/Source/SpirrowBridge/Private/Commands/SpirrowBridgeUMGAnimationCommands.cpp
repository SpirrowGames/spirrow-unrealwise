#include "Commands/SpirrowBridgeUMGAnimationCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Blueprint/UserWidget.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
// Animation support
#include "Animation/WidgetAnimation.h"
#include "MovieScene.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Sections/MovieSceneColorSection.h"
#include "Channels/MovieSceneFloatChannel.h"

FSpirrowBridgeUMGAnimationCommands::FSpirrowBridgeUMGAnimationCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGAnimationCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("create_widget_animation"))
	{
		return HandleCreateWidgetAnimation(Params);
	}
	else if (CommandName == TEXT("add_animation_track"))
	{
		return HandleAddAnimationTrack(Params);
	}
	else if (CommandName == TEXT("add_animation_keyframe"))
	{
		return HandleAddAnimationKeyframe(Params);
	}
	else if (CommandName == TEXT("get_widget_animations"))
	{
		return HandleGetWidgetAnimations(Params);
	}

	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGAnimationCommands::HandleCreateWidgetAnimation(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName, AnimationName, Path = TEXT("/Game/UI");
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("animation_name"), AnimationName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}
	Params->TryGetStringField(TEXT("path"), Path);

	float Length = 1.0f;
	if (Params->HasField(TEXT("length")))
	{
		Length = static_cast<float>(Params->GetNumberField(TEXT("length")));
	}

	bool bLoop = false;
	Params->TryGetBoolField(TEXT("loop"), bLoop);

	// Load Widget Blueprint
	FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
	}

	// Check if animation already exists
	for (UWidgetAnimation* ExistingAnim : WidgetBP->Animations)
	{
		if (ExistingAnim && ExistingAnim->GetFName() == FName(*AnimationName))
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Animation '%s' already exists in Widget Blueprint"), *AnimationName));
		}
	}

	// Create new Widget Animation
	UWidgetAnimation* NewAnimation = NewObject<UWidgetAnimation>(
		WidgetBP,
		UWidgetAnimation::StaticClass(),
		FName(*AnimationName),
		RF_Transactional
	);

	if (!NewAnimation)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Widget Animation"));
	}

	// Create MovieScene for the animation
	UMovieScene* MovieScene = NewObject<UMovieScene>(
		NewAnimation,
		UMovieScene::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	if (!MovieScene)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create MovieScene"));
	}

	NewAnimation->MovieScene = MovieScene;

	// Set playback range
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameNumber StartFrame = 0;
	FFrameNumber EndFrame = FFrameNumber(static_cast<int32>(Length * TickResolution.AsDecimal()));

	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartFrame, EndFrame));
	MovieScene->SetWorkingRange(StartFrame.Value / TickResolution.AsDecimal(), EndFrame.Value / TickResolution.AsDecimal());
	MovieScene->SetViewRange(StartFrame.Value / TickResolution.AsDecimal(), EndFrame.Value / TickResolution.AsDecimal());

	// Set loop mode
	if (bLoop)
	{
		MovieScene->SetEvaluationType(EMovieSceneEvaluationType::WithSubFrames);
		// Note: Loop settings are typically controlled at playback time, not in the MovieScene itself
	}

	// Add animation to Widget Blueprint
	WidgetBP->Animations.Add(NewAnimation);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("animation_name"), AnimationName);
	ResultObj->SetStringField(TEXT("animation_id"), NewAnimation->GetPathName());
	ResultObj->SetNumberField(TEXT("length"), Length);
	ResultObj->SetBoolField(TEXT("loop"), bLoop);

	return ResultObj;
}

// Helper: Find Float Track by Property Name
static UMovieSceneFloatTrack* FindFloatTrackByPropertyName(UMovieScene* MovieScene, const FGuid& BindingGuid, const FString& PropertyName)
{
	if (!MovieScene)
	{
		return nullptr;
	}

	// Find binding by GUID
	const FMovieSceneBinding* Binding = MovieScene->FindBinding(BindingGuid);
	if (!Binding)
	{
		return nullptr;
	}

	// Search through binding's tracks
	for (UMovieSceneTrack* Track : Binding->GetTracks())
	{
		if (UMovieSceneFloatTrack* FloatTrack = Cast<UMovieSceneFloatTrack>(Track))
		{
			// Match by track name or property path
			if (FloatTrack->GetTrackName().ToString() == PropertyName ||
				FloatTrack->GetPropertyPath().ToString() == PropertyName)
			{
				return FloatTrack;
			}
		}
	}

	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGAnimationCommands::HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params)
{
	// Get parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString TargetWidgetName = Params->GetStringField(TEXT("target_widget"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));
	FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

	// Load Widget Blueprint
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
	}

	// Find the animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBP->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}

	if (!Animation)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->GetMovieScene();
	if (!MovieScene)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("MovieScene not found in animation"));
	}

	// Find target widget
	UWidget* TargetWidget = nullptr;
	if (WidgetBP->WidgetTree)
	{
		TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*TargetWidgetName));
	}

	if (!TargetWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Target widget not found: %s"), *TargetWidgetName));
	}

	// Find or create Possessable (binding target)
	FGuid BindingGuid;
	bool bFoundBinding = false;

	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == TargetWidgetName)
		{
			BindingGuid = Possessable.GetGuid();
			bFoundBinding = true;
			break;
		}
	}

	if (!bFoundBinding)
	{
		// Create new Possessable
		BindingGuid = MovieScene->AddPossessable(TargetWidgetName, TargetWidget->GetClass());

		// Add binding to Animation's AnimationBindings
		FWidgetAnimationBinding Binding;
		Binding.WidgetName = FName(*TargetWidgetName);
		Binding.SlotWidgetName = NAME_None;
		Binding.AnimationGuid = BindingGuid;
		Binding.bIsRootWidget = false;
		Animation->AnimationBindings.Add(Binding);
	}

	// Create track based on property
	FString TrackDisplayName;

	if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
	{
		// Float track (Opacity)
		UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (Track)
		{
			Track->SetPropertyNameAndPath(FName("RenderOpacity"), "RenderOpacity");

			// Add section
			UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
			if (Section)
			{
				Section->SetRange(TRange<FFrameNumber>::All());
				Track->AddSection(*Section);
			}

			TrackDisplayName = TEXT("RenderOpacity");
		}
	}
	else if (PropertyName == TEXT("ColorAndOpacity"))
	{
		// Color track
		UMovieSceneColorTrack* Track = MovieScene->AddTrack<UMovieSceneColorTrack>(BindingGuid);
		if (Track)
		{
			Track->SetPropertyNameAndPath(FName("ColorAndOpacity"), "ColorAndOpacity");

			UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Track->CreateNewSection());
			if (Section)
			{
				Section->SetRange(TRange<FFrameNumber>::All());
				Track->AddSection(*Section);
			}

			TrackDisplayName = TEXT("ColorAndOpacity");
		}
	}
	else if (PropertyName.StartsWith(TEXT("RenderTransform.")))
	{
		// RenderTransform property (Translation.X, Translation.Y, Scale.X, Scale.Y, Angle)
		UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create float track for RenderTransform"));
		}

		// Set property name and path
		Track->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);

		// Create section
		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
		if (Section)
		{
			Section->SetRange(TRange<FFrameNumber>::All());
			Track->AddSection(*Section);
		}

		TrackDisplayName = PropertyName;
	}
	else
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported property: %s. Supported: Opacity, RenderOpacity, ColorAndOpacity, RenderTransform.*"), *PropertyName));
	}

	// Save
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create response
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	Response->SetStringField(TEXT("target_widget"), TargetWidgetName);
	Response->SetStringField(TEXT("property_name"), TrackDisplayName);
	Response->SetStringField(TEXT("binding_guid"), BindingGuid.ToString());

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGAnimationCommands::HandleAddAnimationKeyframe(const TSharedPtr<FJsonObject>& Params)
{
	// Get parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString TargetWidgetName = Params->GetStringField(TEXT("target_widget"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));
	double Time = Params->GetNumberField(TEXT("time"));
	FString Interpolation = Params->HasField(TEXT("interpolation")) ?
		Params->GetStringField(TEXT("interpolation")) : TEXT("Linear");
	FString Path = Params->HasField(TEXT("path")) ?
		Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

	// Load Widget Blueprint
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
	}

	// Find the animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBP->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}

	if (!Animation)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->GetMovieScene();
	if (!MovieScene)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("MovieScene not found"));
	}

	// Find Possessable binding GUID
	FGuid BindingGuid;
	bool bFoundBinding = false;

	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == TargetWidgetName)
		{
			BindingGuid = Possessable.GetGuid();
			bFoundBinding = true;
			break;
		}
	}

	if (!bFoundBinding)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("No binding found for widget: %s. Add a track first."), *TargetWidgetName));
	}

	// Calculate frame number
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameNumber FrameNumber = (Time * TickResolution).FloorToFrame();

	// Determine interpolation type
	ERichCurveInterpMode InterpMode = RCIM_Linear;
	if (Interpolation == TEXT("Cubic"))
	{
		InterpMode = RCIM_Cubic;
	}
	else if (Interpolation == TEXT("Constant"))
	{
		InterpMode = RCIM_Constant;
	}

	// Add keyframe based on property
	if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
	{
		// Get Float value
		double Value = Params->GetNumberField(TEXT("value"));

		// Find Float track
		UMovieSceneFloatTrack* Track = MovieScene->FindTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				TEXT("Float track not found. Add a track first using add_animation_track."));
		}

		// Get section
		if (Track->GetAllSections().Num() == 0)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in track"));
		}

		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections()[0]);
		if (!Section)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid section"));
		}

		// Get channel and add keyframe
		FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
		if (Channel)
		{
			FMovieSceneFloatValue KeyValue(Value);
			KeyValue.InterpMode = InterpMode;
			Channel->AddKeys({FrameNumber}, {KeyValue});
		}
	}
	else if (PropertyName == TEXT("ColorAndOpacity"))
	{
		// Get Color value [R, G, B, A]
		const TArray<TSharedPtr<FJsonValue>>* ColorArray;
		if (!Params->TryGetArrayField(TEXT("value"), ColorArray) || ColorArray->Num() < 4)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				TEXT("ColorAndOpacity requires [R, G, B, A] array"));
		}

		float R = (*ColorArray)[0]->AsNumber();
		float G = (*ColorArray)[1]->AsNumber();
		float B = (*ColorArray)[2]->AsNumber();
		float A = (*ColorArray)[3]->AsNumber();

		// Find Color track
		UMovieSceneColorTrack* Track = MovieScene->FindTrack<UMovieSceneColorTrack>(BindingGuid);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				TEXT("Color track not found. Add a track first."));
		}

		if (Track->GetAllSections().Num() == 0)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in color track"));
		}

		UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Track->GetAllSections()[0]);
		if (!Section)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid color section"));
		}

		// Add keyframe to each channel (R, G, B, A)
		TArrayView<FMovieSceneFloatChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
		if (Channels.Num() >= 4)
		{
			FMovieSceneFloatValue RValue(R); RValue.InterpMode = InterpMode;
			FMovieSceneFloatValue GValue(G); GValue.InterpMode = InterpMode;
			FMovieSceneFloatValue BValue(B); BValue.InterpMode = InterpMode;
			FMovieSceneFloatValue AValue(A); AValue.InterpMode = InterpMode;

			Channels[0]->AddKeys({FrameNumber}, {RValue});
			Channels[1]->AddKeys({FrameNumber}, {GValue});
			Channels[2]->AddKeys({FrameNumber}, {BValue});
			Channels[3]->AddKeys({FrameNumber}, {AValue});
		}
	}
	else if (PropertyName.StartsWith(TEXT("RenderTransform.")))
	{
		// RenderTransform property (Translation.X, Translation.Y, Scale.X, Scale.Y, Angle)
		double Value = Params->GetNumberField(TEXT("value"));

		// Find Float track by property name
		UMovieSceneFloatTrack* Track = FindFloatTrackByPropertyName(MovieScene, BindingGuid, PropertyName);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Float track not found for property: %s. Add a track first."), *PropertyName));
		}

		// Get section
		if (Track->GetAllSections().Num() == 0)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in track"));
		}

		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections()[0]);
		if (!Section)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid section"));
		}

		// Get channel and add keyframe
		FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
		if (Channel)
		{
			FMovieSceneFloatValue KeyValue(Value);
			KeyValue.InterpMode = InterpMode;
			Channel->AddKeys({FrameNumber}, {KeyValue});
		}
	}
	else
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported property for keyframe: %s"), *PropertyName));
	}

	// Save
	WidgetBP->MarkPackageDirty();

	// Response
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	Response->SetStringField(TEXT("target_widget"), TargetWidgetName);
	Response->SetStringField(TEXT("property_name"), PropertyName);
	Response->SetNumberField(TEXT("time"), Time);
	Response->SetNumberField(TEXT("frame"), FrameNumber.Value);
	Response->SetStringField(TEXT("interpolation"), Interpolation);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGAnimationCommands::HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params)
{
	// Get parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString Path = Params->HasField(TEXT("path")) ?
		Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

	// Load Widget Blueprint
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
	}

	// Build animations array
	TArray<TSharedPtr<FJsonValue>> AnimationsArray;

	for (UWidgetAnimation* Animation : WidgetBP->Animations)
	{
		if (!Animation) continue;

		TSharedPtr<FJsonObject> AnimObj = MakeShareable(new FJsonObject());
		AnimObj->SetStringField(TEXT("name"), Animation->GetName());

		UMovieScene* MovieScene = Animation->GetMovieScene();
		if (MovieScene)
		{
			// Get animation length
			FFrameRate TickResolution = MovieScene->GetTickResolution();
			FFrameRate DisplayRate = MovieScene->GetDisplayRate();
			TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();

			double StartTime = TickResolution.AsSeconds(PlaybackRange.GetLowerBoundValue());
			double EndTime = TickResolution.AsSeconds(PlaybackRange.GetUpperBoundValue());
			double Length = EndTime - StartTime;

			AnimObj->SetNumberField(TEXT("length"), Length);

			// Check if looping (this info is typically stored externally, default to false)
			AnimObj->SetBoolField(TEXT("is_looping"), false);

			// Get tracks
			TArray<TSharedPtr<FJsonValue>> TracksArray;

			for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
			{
				const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
				FGuid BindingGuid = Possessable.GetGuid();
				FString TargetName = Possessable.GetName();

				// Check for Float tracks (Opacity)
				if (UMovieSceneFloatTrack* FloatTrack = MovieScene->FindTrack<UMovieSceneFloatTrack>(BindingGuid))
				{
					TSharedPtr<FJsonObject> TrackObj = MakeShareable(new FJsonObject());
					TrackObj->SetStringField(TEXT("target_widget"), TargetName);
					TrackObj->SetStringField(TEXT("property"), FloatTrack->GetTrackName().ToString());
					TrackObj->SetStringField(TEXT("type"), TEXT("Float"));

					int32 KeyframeCount = 0;
					if (FloatTrack->GetAllSections().Num() > 0)
					{
						UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(FloatTrack->GetAllSections()[0]);
						if (Section)
						{
							FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
							if (Channel)
							{
								KeyframeCount = Channel->GetNumKeys();
							}
						}
					}
					TrackObj->SetNumberField(TEXT("keyframe_count"), KeyframeCount);

					TracksArray.Add(MakeShareable(new FJsonValueObject(TrackObj)));
				}

				// Check for Color tracks (ColorAndOpacity)
				if (UMovieSceneColorTrack* ColorTrack = MovieScene->FindTrack<UMovieSceneColorTrack>(BindingGuid))
				{
					TSharedPtr<FJsonObject> TrackObj = MakeShareable(new FJsonObject());
					TrackObj->SetStringField(TEXT("target_widget"), TargetName);
					TrackObj->SetStringField(TEXT("property"), ColorTrack->GetTrackName().ToString());
					TrackObj->SetStringField(TEXT("type"), TEXT("Color"));

					int32 KeyframeCount = 0;
					if (ColorTrack->GetAllSections().Num() > 0)
					{
						UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(ColorTrack->GetAllSections()[0]);
						if (Section)
						{
							// Get keyframe count from first channel (R)
							TArrayView<FMovieSceneFloatChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
							if (Channels.Num() > 0)
							{
								KeyframeCount = Channels[0]->GetNumKeys();
							}
						}
					}
					TrackObj->SetNumberField(TEXT("keyframe_count"), KeyframeCount);

					TracksArray.Add(MakeShareable(new FJsonValueObject(TrackObj)));
				}
			}

			AnimObj->SetNumberField(TEXT("track_count"), TracksArray.Num());
			AnimObj->SetArrayField(TEXT("tracks"), TracksArray);
		}
		else
		{
			AnimObj->SetNumberField(TEXT("length"), 0.0);
			AnimObj->SetBoolField(TEXT("is_looping"), false);
			AnimObj->SetNumberField(TEXT("track_count"), 0);
			AnimObj->SetArrayField(TEXT("tracks"), TArray<TSharedPtr<FJsonValue>>());
		}

		AnimationsArray.Add(MakeShareable(new FJsonValueObject(AnimObj)));
	}

	// Response
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetNumberField(TEXT("animation_count"), AnimationsArray.Num());
	Response->SetArrayField(TEXT("animations"), AnimationsArray);

	return Response;
}
