// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSGameMode.h"
#include "MultiplayerFPSGame/Character/FPSCharacter.h"
#include "MultiplayerFPSGame/PlayerController/FPSPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "MultiplayerFPSGame/PlayerState/FPSPlayerState.h"
#include "MultiplayerFPSGame/GameState/FPSGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AFPSGameMode::AFPSGameMode()
{
	bDelayedStart = true;
}

void AFPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AFPSGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void AFPSGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFPSPlayerController* FPSPlayer = Cast<AFPSPlayerController>(*It);
		if (FPSPlayer)
		{
			FPSPlayer->OnMatchStateSet(MatchState, bTeamsMatch);
		}
	}
}

float AFPSGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}

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

void AFPSGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void AFPSGameMode::PlayerLeftGame(AFPSPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;
	AFPSGameState* FPSGameState = GetGameState<AFPSGameState>();
	if (FPSGameState && FPSGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		FPSGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	AFPSCharacter* CharacterLeaving = Cast<AFPSCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}