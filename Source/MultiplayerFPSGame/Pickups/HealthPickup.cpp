// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "MultiplayerFPSGame/Character/FPSCharacter.h"
#include "MultiplayerFPSGame/FPSComponents/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
	bReplicates = true;
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(OtherActor);
	if (IsValid(FPSCharacter))
	{
		UBuffComponent* Buff = FPSCharacter->GetBuff();
		if (IsValid(Buff))
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}

	Destroy();
}