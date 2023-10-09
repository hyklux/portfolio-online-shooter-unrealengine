// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "FPSGameState.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERFPSGAME_API AFPSGameState : public AGameState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class AFPSPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<AFPSPlayerState*> TopScoringPlayers;

	/** 
	* Teams
	*/

	void RedTeamScores();
	void BlueTeamScores();

	TArray<AFPSPlayerState*> RedTeam;
	TArray<AFPSPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;

	UFUNCTION()
	void OnRep_RedTeamScore();

	UFUNCTION()
	void OnRep_BlueTeamScore();

private:

	float TopScore = 0.f;
};
