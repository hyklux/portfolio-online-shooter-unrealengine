# portfolio-online-shooter-unrealengine
Online shooter game made with Unreal Engine


## Introduction


This is an online shooter game made with Unreal Engine and it's Dedicated Server system.


![online_shooter_1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/f30c6d57-b919-4ca7-8cb9-ae0810f48d5f)


## Implementations
- **Weapons**

  
  - Aiming
  - HitScan Weapon (Assault Rifle / Sniper Rifle / Pistol)
  - Shotgun Weapon
  - Projectile Weapon (Rocket Launcher / Grenade Launcher)
  - Throw Grenade


- **Pickups**
  - AmmoPickup
  - HealthPickup
  - ShieldPickup


- **Health & Death**
  - Health
  - Death


- **Lag Compensation in combat**
  - Client-Side
  - Server-Side




## Weapons


Weapon class is the base class for all weapon classes. HitScanWeapon class and ProjectileWeapon class inherit Weapon class and override **Fire(const FVector& HitTarget)** function according to the characteristics of each weapon class.


The diagram shows the inheritance relationships of all weapon classes:
![online_shooter_weapon](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/6cf2dfd2-81b8-412b-933d-0c877b514051)


### Aiming
In every frame **TraceUnderCrosshairs(FHitResult& TraceHitResult)** is called to detect a hit target. If a valid hit target is detected, the crosshairs turn red.
  

![online_shooter_aim_1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/e6a8e8fd-b78b-4e6a-8259-168291d1af0d)
``` c++
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Character) && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		if (IsValid(Character))
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}
```


### Hitscan Weapon (Assault Rifle / Sniper Rifle / Pistol)
Hitscan weapon is a type of weapon that determines whether an object has been hit or not by scanning if the weapon was aimed directly at its target.
When **AHitScanWeapon::Fire(const FVector& HitTarget)** is called, damage is applied instantly to the HitTarget parameter.


**Assault Rifle**


![online_shooter_ar1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/6826d0a2-a7e4-44c7-bb1d-cd1dbbf3c97e)


**Sniper Rifle**


![online_shooter_sniper1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/549c6cba-5efc-406a-b61f-ad71fee6a783)


**Pistol**


![online_shooter_pistol1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/f36a995e-00be-4fd7-9c5a-17e40d8f3a1c)


```c++
void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (IsValid(MuzzleFlashSocket))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(FireHit.GetActor());
		if (IsValid(FPSCharacter) && IsValid(InstigatorController))
		{
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			if (HasAuthority() && bCauseAuthDamage)
			{
				const float DamageToCause = FireHit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;

				UGameplayStatics::ApplyDamage(
					FPSCharacter,
					DamageToCause,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
			if (!HasAuthority() && bUseServerSideRewind)
			{
				FPSOwnerCharacter = FPSOwnerCharacter == nullptr ? Cast<AFPSCharacter>(OwnerPawn) : FPSOwnerCharacter;
				FPSOwnerController = FPSOwnerController == nullptr ? Cast<AFPSPlayerController>(InstigatorController) : FPSOwnerController;
				if (IsValid(FPSOwnerController) && IsValid(FPSOwnerCharacter) && FPSOwnerCharacter->GetLagCompensation() && FPSOwnerCharacter->IsLocallyControlled())
				{
					FPSOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						FPSCharacter,
						Start,
						HitTarget,
						FPSOwnerController->GetServerTime() - FPSOwnerController->SingleTripTime
					);
				}
			}
		}
	}
}
```

### Shotgun
Shotgun is a type of weapon that executes multiple hit scans to the HitTarget at a single fire. Shotgun class inherits HitScanWeapon class as the basic mechanism is the same,
however executes the number of hit scans designated in the BP_Shotgun class.


![online_shooter_shotgun1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/a59706ae-83b0-4dec-8e8c-0529c69d86ae)


``` c++
void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);
	}
}

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;
	
	AController* InstigatorController = OwnerPawn->GetController();
	if (!IsValid(InstigatorController)) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (IsValid(MuzzleFlashSocket))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		// Maps hit character to number of times hit
		TMap<AFPSCharacter*, uint32> HitMap;
		TMap<AFPSCharacter*, uint32> HeadShotHitMap;
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(FireHit.GetActor());
			if (IsValid(FPSCharacter))
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");

				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(FPSCharacter)) HeadShotHitMap[FPSCharacter]++;
					else HeadShotHitMap.Emplace(FPSCharacter, 1);
				}
				else
				{
					if (HitMap.Contains(FPSCharacter)) HitMap[FPSCharacter]++;
					else HitMap.Emplace(FPSCharacter, 1);
				}


				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}
				if (HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						HitSound,
						FireHit.ImpactPoint,
						.5f,
						FMath::FRandRange(-.5f, .5f)
					);
				}
			}
		}
		TArray<AFPSCharacter*> HitCharacters;

		// Maps Character hit to total damage
		TMap<AFPSCharacter*, float> DamageMap;

		// Calculate body shot damage by multiplying times hit x Damage - stored in DamageMap
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);

				HitCharacters.AddUnique(HitPair.Key);
			}
		}

		// Calculate head shot damage by multiplying times hit x HeadShotDamage - stored in DamageMap
		for (auto HeadShotHitPair : HeadShotHitMap)
		{
			if (HeadShotHitPair.Key)
			{
				if (DamageMap.Contains(HeadShotHitPair.Key)) DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
				else DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key);
			}
		}

		// Loop through DamageMap to get total damage for each character
		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key, 
						DamagePair.Value,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}

		if (!HasAuthority() && bUseServerSideRewind)
		{
			FPSOwnerCharacter = FPSOwnerCharacter == nullptr ? Cast<AFPSCharacter>(OwnerPawn) : FPSOwnerCharacter;
			FPSOwnerController = FPSOwnerController == nullptr ? Cast<AFPSPlayerController>(InstigatorController) : FPSOwnerController;
			if (IsValid(FPSOwnerController) && IsValid(FPSOwnerCharacter) && FPSOwnerCharacter->GetLagCompensation() && FPSOwnerCharacter->IsLocallyControlled())
			{
				FPSOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					FPSOwnerController->GetServerTime() - FPSOwnerController->SingleTripTime
				);
			}
		}
	}
}
```

### Projectile Weapon (Rocket Launcher / Grenade Launcher)
A projectile weapon is a type of weapon that works by launching solid projectiles.
When **AProjectileWeapon::Fire(const FVector& HitTarget)** is called, a projectile is spawned and it moves towards the HitTarget parameter.
Damage will be applied to the HitTarget only if the projectile actually his the HitTarget.


**Rocket Launcher**


![online_shooter_rocketlauncher1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/b377467d-1f98-42d7-bc43-7abaf795830b)


**Grenade Launcher**


![online_shooter_grenade_launcher1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/95bd19cf-1100-4d58-8899-5c72c0bd64b0)


``` c++
void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!IsValid(InstigatorPawn)) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();
	if (IsValid(MuzzleFlashSocket) && IsValid(World))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority()) // server
			{
				if (InstigatorPawn->IsLocallyControlled()) // server, host - use replicated projectile
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					if (IsValid(SpawnedProjectile))
					{
						SpawnedProjectile->bUseServerSideRewind = false;
						SpawnedProjectile->Damage = Damage;
						SpawnedProjectile->HeadShotDamage = HeadShotDamage;
					}
				}
				else // server, not locally controlled - spawn non-replicated projectile, SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					if (IsValid(SpawnedProjectile))
					{
						SpawnedProjectile->bUseServerSideRewind = true;
					}
				}
			}
			else // client, using SSR
			{
				if (InstigatorPawn->IsLocallyControlled()) // client, locally controlled - spawn non-replicated projectile, use SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					if (IsValid(SpawnedProjectile))
					{
						SpawnedProjectile->bUseServerSideRewind = true;
						SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
						SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					}
				}
				else // client, not locally controlled - spawn non-replicated projectile, no SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					if (IsValid(SpawnedProjectile))
					{
						SpawnedProjectile->bUseServerSideRewind = false;
					}
				}
			}
		}
		else // weapon not using SSR
		{
			if (InstigatorPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				if (IsValid(SpawnedProjectile))
				{
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
			}
		}
	}
}
```


### Throw Grenade
Throw grenade is different from other weapons. When throw grenade command is pressed, CombatComponent simply creates a grenade projectile and throws it towards the target. 


![online_shooter_throw_grenade1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/28b1049f-f43d-4380-a0c8-e17cb2db1c24)


``` c++
void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0) return;
	if (CombatState != ECombatState::ECS_Unoccupied || !IsValid(EquippedWeapon)) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	
	if (IsValid(Character))
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	
	if (IsValid(Character() && !Character->HasAuthority())
	{
		ServerThrowGrenade();
	}
	
	if (IsValid(Character) && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	
	if (IsValid(Character))
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (IsValid(Character) && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (IsValid(Character) && GrenadeClass && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (IsValid(World))
		{
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParams
				);
		}
	}
}
```

## Pickups
Pickup class is the base class for all pickup classes. OverlapSphere component triggers OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) function when it overlaps with a player. This function is overrided in each inherited pickup class to implement specific features.


The diagram shows the inheritance relationships of all pickup classes:


![online_shooter_pickup_1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/79896da3-c8a5-4d62-97ef-7957f74951ec)


### AmmoPickup
AmmoPickup fills the ammo of a certain weapon with the amount specified in the ammo pickup blueprint class.


![online_shooter_ammo_pickup1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/1679210c-be19-46dc-82a8-a9672513324a)


``` c++
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
```

### HealthPickup
HealthPickup heals player with the amount specified in the health pickup blueprint class.


![online_shooter_health_pickup1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/5f51788c-ebf4-4210-a3a3-531cc553d9c4)


``` c++
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
```

### ShieldPickup
ShieldPickup recharges the shield with the amount specified in the shield pickup blueprint class.


![online_shooter_shield_pickup1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/ac92147e-c020-4ffa-8c34-b454ef4112f8)


``` c++
void AShieldPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(OtherActor);
	if (IsValid(FPSCharacter))
	{
		UBuffComponent* Buff = FPSCharacter->GetBuff();
		if (IsValid(Buff))
		{
			Buff->RechargeShield(ShieldRechargeAmount, ShieldRechargeTime);
		}
	}

	Destroy();
}
```

## Health & Death


### Health
When player is hit, damage is applied by ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser) function. When damage is applied, health and shield value is updated on the HUD. Also, the hit reaction animation is played.


![online_shooter_health1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/ceb96db6-ca9d-4e79-bd15-c4e2233d75d7)


``` c++
void AFPSCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	FPSGameMode = FPSGameMode == nullptr ? GetWorld()->GetAuthGameMode<AFPSGameMode>() : FPSGameMode;
	if (bElimmed || !IsValid(FPSGameMode)) return;
	Damage = FPSGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		if (IsValid(FPSGameMode))
		{
			FPSPlayerController = FPSPlayerController == nullptr ? Cast<AFPSPlayerController>(Controller) : FPSPlayerController;
			AFPSPlayerController* AttackerController = Cast<AFPSPlayerController>(InstigatorController);
			if (IsValid(FPSPlayerController) && IsValid(AttackerController))
			{
				FPSGameMode->PlayerEliminated(this, FPSPlayerController, AttackerController);
			}
		}
	}
}
```
### Death
When player's health is 0, it gets eliminated and the game stats get updated in the custom GameMode class.


![online_shooter_death1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/8ca35223-c985-4ea4-8c2f-f3f8c1bdaf8a)


``` c++
void AFPSGameMode::PlayerEliminated(class AFPSCharacter* ElimmedCharacter, class AFPSPlayerController* VictimController, AFPSPlayerController* AttackerController)
{
	if (!IsValid(AttackerController) || !IsValid(AttackerController->PlayerState)) return;
	if (!IsValid(VictimController) || !IsValid(VictimController->PlayerState)) return;
	AFPSPlayerState* AttackerPlayerState = AttackerController ? Cast<AFPSPlayerState>(AttackerController->PlayerState) : nullptr;
	AFPSPlayerState* VictimPlayerState = VictimController ? Cast<AFPSPlayerState>(VictimController->PlayerState) : nullptr;

	AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();

	if (IsValid(AttackerPlayerState) && AttackerPlayerState != VictimPlayerState && IsValid(FPSGameState))
	{
		TArray<AFPSPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : FPSGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}

		AttackerPlayerState->AddToScore(1.f);
		FPSGameState->UpdateTopScore(AttackerPlayerState);
		if (FPSGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			AFPSCharacter* Leader = Cast<AFPSCharacter>(AttackerPlayerState->GetPawn());
			if (IsValid(Leader))
			{
				Leader->MulticastGainedTheLead();
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (!FPSGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				AFPSCharacter* Loser = Cast<AFPSCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (IsValid(Loser))
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}
	if (IsValid(VictimPlayerState))
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (IsValid(ElimmedCharacter))
	{
		ElimmedCharacter->Elim(false);
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFPSPlayerController* FPSPlayer = Cast<AFPSPlayerController>(*It);
		if (IsValid(FPSPlayer) && AttackerPlayerState && VictimPlayerState)
		{
			FPSPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}
```


## Lag Compensation


### Client-Side




**1. Local fire animations and fire effects**


When pressing FIRE command as a local player, waiting for the multicast RPC to be executed on the server and down to the client could result in bad responsiveness. So for the local player who fired the weapon, fire animations and fire effects are played locally first before executing the multicast RPC from server.


The diagram shows how this works.


![online_shooter_lag_compensation1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/10031b35-5740-47dd-bb18-e3311d5ab790)
``` c++
void UCombatComponent::FireHitScanWeapon()
{
	if (IsValid(EquippedWeapo)n && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (!IsValid(EquippedWeapon)) return;
	if (IsValid(Character) && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (IsValid(Character) && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalFire(TraceHitTarget);
}
```




**2. Ammo update using reconciliation**


``` c++
void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		++Sequence;
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	FPSOwnerCharacter = FPSOwnerCharacter == nullptr ? Cast<AFPSCharacter>(GetOwner()) : FPSOwnerCharacter;
	if (IsValid(FPSOwnerCharacter) && FPSOwnerCharacter->GetCombat() && IsFull())
	{
		FPSOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}
```



**3. Reloading**
Reloading lag compensation is similar as that of fire animation and fire effects. If locally controlled player, reload animation is played locally first. The ammo value is updated on the server and then other clients play the reload animation.


![online_shooter_lag_compensation3](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/382022a7-019a-4a88-b83a-feb0dde7ed5f)


``` c++
void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull() && !bLocallyReloading)
	{
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!IsValid(Character) || !IsValid(EquippedWeapon)) return;

	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled()) HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	bLocallyReloading = false;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::HandleReload()
{
	if (Character)
	{
		Character->PlayReloadMontage();
	}
}
```


### Server-Side


**1. Hit detection using rewind**


**2. Confirming the hit**
