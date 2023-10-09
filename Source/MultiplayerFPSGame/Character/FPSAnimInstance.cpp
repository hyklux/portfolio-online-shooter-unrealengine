// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSAnimInstance.h"
#include "FPSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MultiplayerFPSGame/Weapon/Weapon.h"
#include "MultiplayerFPSGame/FPSTypes/CombatState.h"

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	FPSCharacter = Cast<AFPSCharacter>(TryGetPawnOwner());
}

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (FPSCharacter == nullptr)
	{
		FPSCharacter = Cast<AFPSCharacter>(TryGetPawnOwner());
	}
	if (FPSCharacter == nullptr) return;

	FVector Velocity = FPSCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = FPSCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = FPSCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = FPSCharacter->IsWeaponEquipped();
	EquippedWeapon = FPSCharacter->GetEquippedWeapon();
	bIsCrouched = FPSCharacter->bIsCrouched;
	bAiming = FPSCharacter->IsAiming();
	TurningInPlace = FPSCharacter->GetTurningInPlace();
	bRotateRootBone = FPSCharacter->ShouldRotateRootBone();
	bElimmed = FPSCharacter->IsElimmed();
	bHoldingTheFlag = FPSCharacter->IsHoldingTheFlag();

	// Offset Yaw for Strafing
	FRotator AimRotation = FPSCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(FPSCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = FPSCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = FPSCharacter->GetAO_Yaw();
	AO_Pitch = FPSCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && FPSCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		FPSCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (FPSCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - FPSCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
			FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
			FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), FPSCharacter->GetHitTarget(), FColor::Orange);
		}
	}

	bUseFABRIK = FPSCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	bool bFABRIKOverride = FPSCharacter->IsLocallyControlled() &&
		FPSCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade &&
		FPSCharacter->bFinishedSwapping;
	if (bFABRIKOverride)
	{
		bUseFABRIK = !FPSCharacter->IsLocallyReloading();
	}
	bUseAimOffsets = FPSCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !FPSCharacter->GetDisableGameplay();
	bTransformRightHand = FPSCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !FPSCharacter->GetDisableGameplay();
}
