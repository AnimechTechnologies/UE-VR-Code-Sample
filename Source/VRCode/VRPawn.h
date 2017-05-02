// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"

#include "VRPawn.generated.h"

UCLASS()
class VRCODE_API AVRPawn : public APawn
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent *VROrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent *Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UChildActorComponent *LeftHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UChildActorComponent *RightHand;

	void HandleStickInputStyleTeleportActivation( FVector2D AxisInput, class AVRHand *Current, class AVRHand *Other );
	bool GetRotationFromInput( FVector2D AxisInput, FRotator &OrientRotator );

public:

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Default" )
	uint8 DeviceType;

	// Sets default values for this pawn's properties
	AVRPawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent( class UInputComponent* InputComponent ) override;
	
	void UpdateGrip( UChildActorComponent *hand, bool pressed );

	void GripLeft();
	void StopGripLeft();
	void GripRight();
	void StopGripRight();


	UFUNCTION( BlueprintNativeEvent, BlueprintCallable, Category = "Default" )
	void ExecuteTeleport();

	
};
