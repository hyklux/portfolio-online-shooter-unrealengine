# portfolio-online-shooter-unrealengine
Online shooter game made with Unreal Engine


## Introduction
Youtube Link : https://youtu.be/NV9-pcHnVTc


This is an online shooter game made with Unreal Engine and it's Dedicated Server system.


![online_shooter_1](https://github.com/hyklux/portfolio-online-shooter-unrealengine/assets/96270683/f30c6d57-b919-4ca7-8cb9-ae0810f48d5f)


## Implementations
- **Weapons**

  
  - Aiming
  - HitScan Weapon (Assault Rifle / Sniper Rifle / Pistol)
  - Projectile Weapon (Rocket Launcher / Grenade Launcher)
  - Shotgun Weapon (Shotgun)
  - Grenade


- **Pickups**
  - Weapons
  - Ammos
  - Health
  - Shield


- **Health & Death**
  - Health
  - Death


- **Lag Compensation**
  - Client-Side Prediction
  - Server-Side Prediction




## Weapons

### Aiming
For everyframe **TraceUnderCrosshairs(FHitResult& TraceHitResult)** is called to detect a hit target. If a valid hit target is detected crosshairs turn red.
  

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


### HitScan Weapon (Assault Rifle / Sniper Rifle / Pistol)


### Projectile Weapon (Rocket Launcher / Grenade Launcher)


### Shotgun Weapon (Shotgun)


## Pickups


### Weapons


### Ammos


### Health


### Shield


## Health & Death


### Health


### Death

## Lag Compensation


### Client-Side Prediction


### Server-Side Prediction
