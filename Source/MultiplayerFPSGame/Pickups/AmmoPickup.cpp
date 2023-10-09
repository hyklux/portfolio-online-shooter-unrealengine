// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "MultiplayerFPSGame/Character/FPSCharacter.h"
#include "MultiplayerFPSGame/FPSComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(OtherActor);
	if (IsValid(FPSCharacter))
	{
		UCombatComponent* Combat = FPSCharacter->GetCombat();
		if (IsValid(Combat))
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}