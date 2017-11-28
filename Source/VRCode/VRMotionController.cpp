/*
 * Copyright (c) 2017 Skyler Clark. All rights reserved.
 *
 * Licensed under the MIT License. See License.md file in the
 * project root for full license information.
 *
 * Author: Skyler Clark (@sclark39)
 * Website: http://skylerclark.com
*
* Github: https://github.com/sclark39/UE-VR-Code-Sample/
 */

#include "VRCode.h"
#include "VRMotionController.h"
#include "IPickupable.h"
#include "Runtime/HeadMountedDisplay/Public/MotionControllerComponent.h"
#include "Runtime/Engine/Classes/Components/SplineComponent.h"
#include "Runtime/HeadMountedDisplay/Public/IHeadMountedDisplay.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "Runtime/Engine/Classes/Components/SplineMeshComponent.h"
#include "SteamVRChaperoneComponent.h"

// Sets default values
AVRMotionController::AVRMotionController() :
	Extents( 500, 500, 500 )
{


	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Hand = EControllerHand::Right;
	Grip = EGripState::Open;

	Scene = CreateDefaultSubobject<USceneComponent>( TEXT( "Scene" ) );

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>( TEXT( "MotionController" ) );
	MotionController->SetupAttachment( Scene );
	MotionController->Hand = Hand;

	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>( TEXT( "HandMesh" ) );
	HandMesh->SetupAttachment( MotionController );

	GrabSphere = CreateDefaultSubobject<USphereComponent>( TEXT( "GrabSphere" ) );
	GrabSphere->SetupAttachment( HandMesh );
	GrabSphere->InitSphereRadius( 10.0f );
	GrabSphere->OnComponentBeginOverlap.AddDynamic( this, &AVRMotionController::OnComponentBeginOverlap );

	ArcDirection = CreateDefaultSubobject<UArrowComponent>( TEXT( "ArcDirection" ) );
	ArcDirection->SetupAttachment( HandMesh );

	ArcSpline = CreateDefaultSubobject<USplineComponent>( TEXT( "ArcSpline" ) );
	ArcSpline->SetupAttachment( HandMesh );

	ArcEndPoint = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "ArcEndPoint" ) );
	ArcEndPoint->SetupAttachment( Scene );
	ArcEndPoint->SetVisibility( false );

	TeleportCylinder = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "TeleportCylinder" ) );
	TeleportCylinder->SetupAttachment( Scene );

	TeleportRing = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "TeleportRing" ) );
	TeleportRing->SetupAttachment( TeleportCylinder );

	TeleportArrow = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "TeleportArrow" ) );
	TeleportArrow->SetupAttachment( TeleportCylinder );

	RoomScaleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoomScaleMesh"));
	RoomScaleMesh->SetupAttachment( TeleportArrow );

	SteamVRChaperone = CreateDefaultSubobject<USteamVRChaperoneComponent>(TEXT("SteamVRChaperone"));

}

void AVRMotionController::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);

	if (Hand == EControllerHand::Left)
	{
		// Reflect hand mesh
		HandMesh->SetWorldScale3D( FVector( 1, 1, -1 ) );
	}

}

void AVRMotionController::RumbleController_Implementation( float intensity )
{
	APlayerController *playerController = GetWorld()->GetFirstPlayerController();

	if (HapticEffect == nullptr)
	{
		float duration = 0.04;
		bool affectsLeftLarge = Hand == EControllerHand::Left;
		bool affectsLeftSmall = Hand == EControllerHand::Left;
		bool affectsRightLarge = Hand == EControllerHand::Right;
		bool affectsRightSmall = Hand == EControllerHand::Right;
		auto action = EDynamicForceFeedbackAction::Start;
		FLatentActionInfo actionInfo;
		actionInfo.CallbackTarget = this;

		playerController->PlayDynamicForceFeedback(intensity, duration, affectsLeftLarge, affectsLeftSmall, affectsRightLarge, affectsRightSmall, action, actionInfo);
		return;
	}

	playerController->PlayHapticEffect(HapticEffect, Hand, intensity, false);
}

void AVRMotionController::OnComponentBeginOverlap( UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	Grip = EGripState::CanGrab;
	if ( ( OtherComp != nullptr ) && ( OtherComp != GrabSphere ) )
	{
		UStaticMeshComponent *mesh = Cast<UStaticMeshComponent>( OtherComp );
		if ( mesh && mesh->IsSimulatingPhysics() )
		{
			this->RumbleController( 0.8 );
		}
	}
}

// Called when the game starts or when spawned
void AVRMotionController::BeginPlay()
{
	Super::BeginPlay();

	SetupRoomScaleOutline();

	MotionController->Hand = Hand;
	if ( Hand == EControllerHand::Left )
	{
		// Reflect hand mesh
		MotionController->Hand = Hand;
		HandMesh->SetWorldScale3D( FVector( 1, 1, -1 ) );
	}

	// Hide until activation (but wait for BeginPlay so it is shown in editor)
	TeleportCylinder->SetVisibility( false, true );
}

AActor* AVRMotionController::GetActorNearHand()
{
	TArray<AActor*> overlappingActors;

	GrabSphere->GetOverlappingActors( overlappingActors );
	FVector handLocation = GrabSphere->GetComponentLocation();

	AActor* nearest = nullptr;
	float mindist = 99999999999;

	// Find closest overlaping actor
	for ( AActor *actor : overlappingActors )
	{
		bool isPickupable = actor->GetClass()->ImplementsInterface( UPickupable::StaticClass() );
		if ( isPickupable )
		{

			float dist = ( actor->GetActorLocation() - handLocation ).SizeSquared();
			if ( dist < mindist )
			{
				mindist = dist;
				nearest = actor;
			}
		}
	}

	// 	if ( GEngine && Hand == EControllerHand::Right )
// 		GEngine->AddOnScreenDebugMessage( -1, 0.16f, FColor::Red,
// 			FString::Printf( TEXT( "Actors near right hand %d, found pickupable: %d, %s" ),
// 			overlappingActors.Num(),
// 			count,
// 			nearest ? TEXT( "TRUE" ) : TEXT( "FALSE" ) ) );

	return nearest;
}

void AVRMotionController::UpdateAnimationGripState()
{
	// Default to Open
	Grip = EGripState::Open;

	if ( AttachedActor )
	{
		// If holding an object, always keep fist closed
		Grip = EGripState::Grab;
	}
	else
	{
		// React to player input
		if ( WantsToGrip )
			Grip = EGripState::Grab;

		// If not holding something, the hand should open or close
		// slightly when passing over an interactable object
		AActor *actor = GetActorNearHand();
		if ( actor )
			Grip = EGripState::CanGrab;
	}

	// Only let hand collide with environment while gripping
	if ( Grip == EGripState::Grab )
		HandMesh->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	else
		HandMesh->SetCollisionEnabled( ECollisionEnabled::NoCollision );
}

void AVRMotionController::GrabActor_Implementation()
{
	WantsToGrip = true;

	AActor *actor = GetActorNearHand();
	if ( actor && actor->IsValidLowLevel() )
	{
		AttachedActor = actor;
		IPickupable::Execute_Pickup( actor, MotionController );
		RumbleController( 0.7 );
	}

}

void AVRMotionController::ReleaseActor_Implementation()
{
	WantsToGrip = false;

	AActor *actor = AttachedActor;
	if ( actor && actor->IsValidLowLevel() )
	{
		// Make sure this hand is still holding the Actor (May have been taken by another hand / event)
		if ( MotionController == actor->GetRootComponent()->GetAttachParent() )
		{
			IPickupable::Execute_Drop( actor );
			RumbleController( 0.2 );
		}
	}

	AttachedActor = nullptr;
}

// Called every frame
void AVRMotionController::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	UpdateAnimationGripState();

	UpdateRoomScaleOutline();

	HandleTeleportationArc();
}

void AVRMotionController::ActivateTeleporter()
{
//	if ( GEngine ) GEngine->AddOnScreenDebugMessage( -1, 0.16f, FColor::White, FString::Printf( TEXT( "Activating Teleporter " ) ) );
	IsTeleporterActive = true;

	TeleportCylinder->SetVisibility(true, true);

	TeleportCylinder->SetVisibility(IsRoomScale, false);

	if (MotionController)
		InitialControllerRotation = MotionController->GetComponentRotation();
}


void AVRMotionController::DisableTeleporter()
{
	IsTeleporterActive = false;

	TeleportCylinder->SetVisibility( false, true );
	ArcEndPoint->SetVisibility( false );

	// Hide old Spline
	for ( USplineMeshComponent *SplineMesh : SplineMeshes )
		SplineMesh->SetVisibility( false );

	// TODO: Roomscale Mesh
}

bool AVRMotionController::TraceTeleportDestination( TArray<FVector> &TracePoints, FVector &NavMeshLocation, FVector &TraceLocation )
{

	FVector StartPos = ArcDirection->GetComponentLocation();
	FVector LaunchVelocity = ArcDirection->GetForwardVector();

	LaunchVelocity *= TeleportLaunchVelocity;

	// Predict Projectile Path

	FPredictProjectilePathParams PredictParams( 0.0f, StartPos, LaunchVelocity, 4.0f, UEngineTypes::ConvertToObjectType( ECC_WorldStatic ) );
	FPredictProjectilePathResult PredictResult;
	const bool DidPredictPath = UGameplayStatics::PredictProjectilePath( GetWorld(), PredictParams, PredictResult );
	if ( !DidPredictPath )
		return false;

	// Getting Projected Endpoint

	FVector PointToProject = PredictResult.HitResult.Location;
	FNavLocation ProjectedHitLocation;
	UNavigationSystem *NavSystem = GetWorld()->GetNavigationSystem();
	const bool DidProjectToNav = NavSystem->ProjectPointToNavigation( PointToProject, ProjectedHitLocation, Extents );
	if ( !DidProjectToNav )
		return false;

	// Outputs...

	TracePoints.Empty();
	for ( FPredictProjectilePathPointData Point : PredictResult.PathData )
		TracePoints.Push( Point.Location );

	TraceLocation = PredictResult.HitResult.Location;
	NavMeshLocation = ProjectedHitLocation.Location;

	return true;
}


void AVRMotionController::GetTeleportDestination( FVector &OutPosition, FRotator &OutRotator )
{
	IHeadMountedDisplay *hmd = GEngine->HMDDevice.Get();
	FVector DevicePosition(ForceInitToZero);
	if ( hmd )
	{
		FRotator DeviceRotation;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition( DeviceRotation, DevicePosition );
		DevicePosition.Z = 0; // Ignore relative height difference
	}

	DevicePosition = TeleportRotator.RotateVector( DevicePosition );

	// Substract HMD origin (Camera) to get correct Pawn destination for teleportation.
	OutPosition = TeleportCylinder->GetComponentLocation() - DevicePosition;
	OutRotator = TeleportRotator;
}

FRotator AVRMotionController::GetControllerRelativeRotation()
{
	const FTransform InitialTransform( InitialControllerRotation );
	const FTransform CurrentTransform = MotionController->GetComponentTransform();
	const FTransform RelativeTransform = CurrentTransform.GetRelativeTransform( InitialTransform );

	return RelativeTransform.GetRotation().Rotator();
}

void AVRMotionController::SetupRoomScaleOutline()
{
	auto Vertices = SteamVRChaperone->GetBounds();
	auto Normal = FVector(0.0f, 0.0f, 1.0f);

	FVector RectCenter;
	FRotator RectRotation;
	float SideLengthX, SideLengthY;

	UKismetMathLibrary::MinimumAreaRectangle(GetWorld(), Vertices, Normal, RectCenter, RectRotation, SideLengthX, SideLengthY);

	if (FMath::IsNearlyEqual(SideLengthX, 100.0f, 0.01f) || FMath::IsNearlyEqual(SideLengthY, 100.0f, 0.01f))
	{
		IsRoomScale = false;
		return; // Measure Chaperone (Defaults to 100x100 if roomscale isn't used)
	}

	IsRoomScale = true;

	// Setup Room-scale mesh (1x1x1 units in size by default) to the size of the room-scale dimensions
	RoomScaleMesh->SetWorldScale3D(FVector(SideLengthX, SideLengthY, ChaperoneMeshHeight));
	RoomScaleMesh->SetRelativeRotation(RectRotation);
}

void AVRMotionController::UpdateRoomScaleOutline()
{
	if (RoomScaleMesh->IsVisible())
	{
		FRotator DeviceRotation;
		FVector DevicePosition;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);

		DeviceRotation.Pitch = 0;
		DeviceRotation.Roll = 0;

		DevicePosition.X = -DevicePosition.X;
		DevicePosition.Y = -DevicePosition.Y;
		DevicePosition.Z = 0;

		FVector NewLocation = DeviceRotation.UnrotateVector(DevicePosition);

		RoomScaleMesh->SetRelativeLocation(NewLocation, false, nullptr, ETeleportType::None);
	}
}

void AVRMotionController::HandleTeleportationArc()
{
	ClearArc();

	if (IsTeleporterActive)
	{
		TArray<FVector> TracePoints;
		FVector NavMeshLocation;
		FVector TraceLocation;
		bool IsValidTeleportDestination = TraceTeleportDestination(TracePoints, NavMeshLocation, TraceLocation);

		TeleportCylinder->SetVisibility(IsValidTeleportDestination, true);

		// Trace down to find a valid location for player to stand at (original NavMesh location is offset upwards, so we must find the actual floor)
		FHitResult OutHit;
		{
			static const FName LineTraceSingleName(TEXT("TeleporterDrop"));
			FCollisionQueryParams CollisionQueryParams(LineTraceSingleName, false, this);

			FVector EndPos = NavMeshLocation + FVector(0, 0, -200); // Create downward vector
			GetWorld()->LineTraceSingleByChannel(OutHit, NavMeshLocation, EndPos, ECC_WorldStatic, CollisionQueryParams);
		}

		FVector TeleportCylinderLocation;
		if (OutHit.bBlockingHit)
			TeleportCylinderLocation = OutHit.ImpactPoint;
		else
			TeleportCylinderLocation = NavMeshLocation;

		TeleportCylinder->SetWorldLocation(TeleportCylinderLocation, false, nullptr, ETeleportType::TeleportPhysics);

		// If it changed, rumble.
		if (LastIsValidTeleportDestination != IsValidTeleportDestination)
		{
			RumbleController(0.3);
		}

		LastIsValidTeleportDestination = IsValidTeleportDestination;

		UpdateArcSpline(IsValidTeleportDestination, TracePoints);

		UpdateArcEndpoint(TraceLocation, IsValidTeleportDestination);
	}
}

void AVRMotionController::ClearArc()
{
	ArcSpline->ClearSplinePoints();
}

void AVRMotionController::UpdateArcSpline(bool FoundValidLocation, TArray<FVector> SplinePoints)
{
	if (!FoundValidLocation)
	{
		auto StartPos = ArcDirection->K2_GetComponentLocation();
		auto EndPos = StartPos + ArcDirection->GetForwardVector() * 20;

		SplinePoints.Empty();
		SplinePoints.Add(StartPos);
		SplinePoints.Add(EndPos);
	}

	// Make Spline....
	if (SplinePoints.Num() > 0)
	{
		for (FVector TracePoint : SplinePoints)
		{
			ArcSpline->AddSplinePoint(TracePoint, ESplineCoordinateSpace::Local, true);
		}

		ArcSpline->SetSplinePointType(SplinePoints.Num() - 1, ESplinePointType::CurveClamped, true);

		for (int i = 0; i < SplinePoints.Num() - 1; i++)
		{
			FVector StartPos = SplinePoints[i];
			FVector StartTangent = ArcSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);

			FVector EndPos = SplinePoints[i + 1];
			FVector EndTangent = ArcSpline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);

			USplineMeshComponent *SplineMeshComponent;

			if (i < SplineMeshes.Num())
			{
				SplineMeshComponent = SplineMeshes[i];
			}
			else
			{
				SplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass());
				SplineMeshComponent->SetStaticMesh(BeamMesh);
				SplineMeshComponent->SetMaterial(0, BeamMaterial);
				SplineMeshComponent->SetStartScale(FVector2D(4, 4));
				SplineMeshComponent->SetEndScale(FVector2D(4, 4));
				SplineMeshComponent->SetBoundaryMax(1);
				SplineMeshes.Push(SplineMeshComponent);
			}

			SplineMeshComponent->SetVisibility(true);
			SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		}

		// Hide any extra
		for (int i = SplinePoints.Num() - 1; i < SplineMeshes.Num(); i++)
		{
			SplineMeshes[i]->SetVisibility(false);
		}

		RegisterAllComponents();
	}
}

void AVRMotionController::UpdateArcEndpoint(FVector NewLocation, bool ValidLocationFound)
{
	ArcEndPoint->SetVisibility(ValidLocationFound && IsTeleporterActive, false);
	ArcEndPoint->SetWorldLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	FRotator ArrowRotator = TeleportRotator;

	IHeadMountedDisplay *hmd = GEngine->HMDDevice.Get();
	if (hmd)
	{
		FRotator DeviceRotation;
		FVector DevicePosition;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);

		DeviceRotation.Pitch = 0;
		DeviceRotation.Roll = 0;

		ArrowRotator = UKismetMathLibrary::ComposeRotators(TeleportRotator, DeviceRotation);
	}

	TeleportArrow->SetWorldRotation(ArrowRotator);
}
