/*
* Author: Skyler Clark (@sclark39)
* Website: http://skylerclark.com
* License: MIT License
*/

#pragma once

#include "GameFramework/Pawn.h"

#include "VRMotionControllerPawn.generated.h"

UENUM( BlueprintType )
enum class ETeleportControlScheme : uint8
{
	Auto,				// Select By Device
	ButtonAndStick,		// Press a button
	StickOnly,			// Robo Rally Style
	ControllerRoll		// For PSVR
};

UCLASS()
class VRCODE_API AVRMotionControllerPawn : public APawn
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class USceneComponent *VROrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent *Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class UChildActorComponent *LeftHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class UChildActorComponent *RightHand;

	void HandleStickInputStyleTeleportActivation( FVector2D AxisInput, class AVRMotionController *Current, class AVRMotionController *Other );
	bool GetRotationFromInput( class AVRMotionController *Current, FVector2D AxisInput, FRotator &OrientRotator );

public:

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float FadeInDuration = 0.1;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float FadeOutDuration = 0.1;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	FLinearColor TeleportFadeColor = FLinearColor::Black;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float ThumbDeadzone = 0.7;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float DefaultPlayerHeight = 180;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	ETeleportControlScheme ControlScheme = ETeleportControlScheme::Auto;

	uint8 DeviceType;
	bool IsTeleporting;

	// Sets default values for this pawn's properties
	AVRMotionControllerPawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent( class UInputComponent* InputComponent ) override;

	UFUNCTION()
	void HandleButtonStyleTeleportActivation( class UChildActorComponent *Hand, EInputEvent KeyEvent );

	UFUNCTION()
	void HandleGrip( class UChildActorComponent *Hand, EInputEvent KeyEvent );

	void BindInputActionUFunction( class UInputComponent* PlayerInputComponent, FName ActionName, EInputEvent KeyEvent, FName FuncName, class UChildActorComponent *Hand );

	UFUNCTION()
	void FinishTeleport( class AVRMotionController *Current, const FVector &TeleportPosition, const FRotator &TeleportRotator );

	void ExecuteTeleport( class AVRMotionController *Current );



};
