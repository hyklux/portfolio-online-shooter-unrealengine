// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSGameState.h"
#include "Net/UnrealNetwork.h"
#include "MultiplayerFPSGame/PlayerState/FPSPlayerState.h"
#include "MultiplayerFPSGame/PlayerController/FPSPlayerController.h"

void AFPSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSGameState, TopScoringPlayers);
	DOREPLIFETIME(AFPSGameState, RedTeamScore);
	DOREPLIFETIME(AFPSGameState, BlueTeamScore);
}

void AFPSGameState::UpdateTopScore(class AFPSPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void AFPSGameState::RedTeamScores()
{
	++RedTeamScore;
	AFPSPlayerController* BPlayer = Cast<AFPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void AFPSGameState::BlueTeamScores()
{
	++BlueTeamScore;
	AFPSPlayerController* BPlayer = Cast<AFPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void AFPSGameState::OnRep_RedTeamScore()
{
	AFPSPlayerController* BPlayer = Cast<AFPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void AFPSGameState::OnRep_BlueTeamScore()
{
	AFPSPlayerController* BPlayer = Cast<AFPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
