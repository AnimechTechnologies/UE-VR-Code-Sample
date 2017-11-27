/*
* Author: Skyler Clark (@sclark39)
* Website: http://skylerclark.com
* License: MIT License
*/

#include "VRCode.h"
#include "PickupStaticMesh.h"


// Sets default values
APickupStaticMesh::APickupStaticMesh()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetSimulatePhysics(true);
	StaticMeshComponent->bGenerateOverlapEvents = true;

	StaticMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
}

// Called when the game starts or when spawned
void APickupStaticMesh::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void APickupStaticMesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupStaticMesh::Pickup_Implementation(class USceneComponent *AttachTo)
{
	StaticMeshComponent->SetSimulatePhysics(false);
	USceneComponent *Root = GetRootComponent();

	FAttachmentTransformRules AttachmentTransformRules(EAttachmentRule::KeepWorld, false);
	Root->AttachToComponent(AttachTo, AttachmentTransformRules);
}

void APickupStaticMesh::Drop_Implementation()
{
	StaticMeshComponent->SetSimulatePhysics(true);

	FDetachmentTransformRules DetatchmentTransformRules(EDetachmentRule::KeepWorld, true);
	DetachFromActor(DetatchmentTransformRules);
}
