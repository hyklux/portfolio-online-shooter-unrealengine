// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSPlayerController.h"
#include "MultiplayerFPSGame/HUD/FPSHUD.h"
#include "MultiplayerFPSGame/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "MultiplayerFPSGame/Character/FPSCharacter.h"
#include "Net/UnrealNetwork.h"
#include "MultiplayerFPSGame/GameMode/FPSGameMode.h"
#include "MultiplayerFPSGame/PlayerState/FPSPlayerState.h"
#include "MultiplayerFPSGame/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "MultiplayerFPSGame/FPSComponents/CombatComponent.h"
#include "MultiplayerFPSGame/GameState/FPSGameState.h"
#include "Components/Image.h"
#include "MultiplayerFPSGame/HUD/ReturnToMainMenu.h"
#include "MultiplayerFPSGame/FPSTypes/Announcement.h"

void AFPSPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void AFPSPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
		if (FPSHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				FPSHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			if (Victim == Self && Attacker != Self)
			{
				FPSHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "you");
				return;
			}
			if (Attacker == Victim && Attacker == Self)
			{
				FPSHUD->AddElimAnnouncement("You", "yourself");
				return;
			}
			if (Attacker == Victim && Attacker != Self)
			{
				FPSHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
				return;
			}
			FPSHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}

void AFPSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FPSHUD = Cast<AFPSHUD>(GetHUD());
	ServerCheckMatchState();
}

void AFPSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSPlayerController, MatchState);
	DOREPLIFETIME(AFPSPlayerController, bShowTeamScores);
}

void AFPSPlayerController::HideTeamScores()
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->RedTeamScore &&
		FPSHUD->CharacterOverlay->BlueTeamScore &&
		FPSHUD->CharacterOverlay->ScoreSpacerText;
	if (bHUDValid)
	{
		FPSHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		FPSHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		FPSHUD->CharacterOverlay->ScoreSpacerText->SetText(FText());
	}
}

void AFPSPlayerController::InitTeamScores()
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->RedTeamScore &&
		FPSHUD->CharacterOverlay->BlueTeamScore &&
		FPSHUD->CharacterOverlay->ScoreSpacerText;
	if (bHUDValid)
	{
		FString Zero("0");
		FString Spacer("|");
		FPSHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));
		FPSHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
		FPSHUD->CharacterOverlay->ScoreSpacerText->SetText(FText::FromString(Spacer));
	}
}

void AFPSPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->RedTeamScore;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
		FPSHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void AFPSPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->BlueTeamScore;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
		FPSHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void AFPSPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
}

void AFPSPlayerController::CheckPing(float DeltaTime)
{
	if (HasAuthority()) return;
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			if (PlayerState->GetPing() * 4 > HighPingThreshold) // ping is compressed; it's actually ping / 4
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		FPSHUD && FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->HighPingAnimation &&
		FPSHUD->CharacterOverlay->IsAnimationPlaying(FPSHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void AFPSPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void AFPSPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
	}
}

// Is the ping too high?
void AFPSPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void AFPSPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AFPSPlayerController::HighPingWarning()
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->HighPingImage &&
		FPSHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		FPSHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		FPSHUD->CharacterOverlay->PlayAnimation(
			FPSHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5);
	}
}

void AFPSPlayerController::StopHighPingWarning()
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->HighPingImage &&
		FPSHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		FPSHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (FPSHUD->CharacterOverlay->IsAnimationPlaying(FPSHUD->CharacterOverlay->HighPingAnimation))
		{
			FPSHUD->CharacterOverlay->StopAnimation(FPSHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void AFPSPlayerController::ServerCheckMatchState_Implementation()
{
	AFPSGameMode* GameMode = Cast<AFPSGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AFPSPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (FPSHUD && MatchState == MatchState::WaitingToStart)
	{
		FPSHUD->AddAnnouncement();
	}
}

void AFPSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(InPawn);
	if (FPSCharacter)
	{
		SetHUDHealth(FPSCharacter->GetHealth(), FPSCharacter->GetMaxHealth());
	}
}

void AFPSPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->HealthBar &&
		FPSHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		FPSHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		FPSHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AFPSPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->ShieldBar &&
		FPSHUD->CharacterOverlay->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		FPSHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		FPSHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void AFPSPlayerController::SetHUDScore(float Score)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->ScoreAmount;

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		FPSHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void AFPSPlayerController::SetHUDDefeats(int32 Defeats)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		FPSHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void AFPSPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		FPSHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void AFPSPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		FPSHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void AFPSPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			FPSHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		FPSHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AFPSPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->Announcement &&
		FPSHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			FPSHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		FPSHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void AFPSPlayerController::SetHUDGrenades(int32 Grenades)
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	bool bHUDValid = FPSHUD &&
		FPSHUD->CharacterOverlay &&
		FPSHUD->CharacterOverlay->GrenadesText;
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		FPSHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void AFPSPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	
	if (HasAuthority())
	{
		if (FPSGameMode == nullptr)
		{
			FPSGameMode = Cast<AFPSGameMode>(UGameplayStatics::GetGameMode(this));
			LevelStartingTime = FPSGameMode->LevelStartingTime;
		}
		FPSGameMode = FPSGameMode == nullptr ? Cast<AFPSGameMode>(UGameplayStatics::GetGameMode(this)) : FPSGameMode;
		if (FPSGameMode)
		{
			SecondsLeft = FMath::CeilToInt(FPSGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void AFPSPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (FPSHUD && FPSHUD->CharacterOverlay)
		{
			CharacterOverlay = FPSHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(GetPawn());
				if (FPSCharacter && FPSCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(FPSCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void AFPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &AFPSPlayerController::ShowReturnToMainMenu);

}

void AFPSPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AFPSPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AFPSPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AFPSPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AFPSPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AFPSPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AFPSPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
	if (HasAuthority()) bShowTeamScores = bTeamsMatch;
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	if (FPSHUD)
	{
		if (FPSHUD->CharacterOverlay == nullptr) FPSHUD->AddCharacterOverlay();
		if (FPSHUD->Announcement)
		{
			FPSHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
		if (!HasAuthority()) return;
		if (bTeamsMatch)
		{
			InitTeamScores();
		}
		else
		{
			HideTeamScores();
		}
	}
}

void AFPSPlayerController::HandleCooldown()
{
	FPSHUD = FPSHUD == nullptr ? Cast<AFPSHUD>(GetHUD()) : FPSHUD;
	if (FPSHUD)
	{
		FPSHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = FPSHUD->Announcement && 
			FPSHUD->Announcement->AnnouncementText && 
			FPSHUD->Announcement->InfoText;

		if (bHUDValid)
		{
			FPSHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			FPSHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			AFPSGameState* FPSGameState = Cast<AFPSGameState>(UGameplayStatics::GetGameState(this));
			AFPSPlayerState* FPSPlayerState = GetPlayerState<AFPSPlayerState>();
			if (FPSGameState && FPSPlayerState)
			{
				TArray<AFPSPlayerState*> TopPlayers = FPSGameState->TopScoringPlayers;
				FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(FPSGameState) : GetInfoText(TopPlayers);

				FPSHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(GetPawn());
	if (FPSCharacter && FPSCharacter->GetCombat())
	{
		FPSCharacter->bDisableGameplay = true;
		FPSCharacter->GetCombat()->FireButtonPressed(false);
	}
}

FString AFPSPlayerController::GetInfoText(const TArray<class AFPSPlayerState*>& Players)
{
	AFPSPlayerState* FPSPlayerState = GetPlayerState<AFPSPlayerState>();
	if (FPSPlayerState == nullptr) return FString();
	FString InfoTextString;
	if (Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == FPSPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *Players[0]->GetPlayerName());
	}
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		for (auto TiedPlayer : Players)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}

	return InfoTextString;
}

FString AFPSPlayerController::GetTeamsInfoText(AFPSGameState* FPSGameState)
{
	if (FPSGameState == nullptr) return FString();
	FString InfoTextString;

	const int32 RedTeamScore = FPSGameState->RedTeamScore;
	const int32 BlueTeamScore = FPSGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if (BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
	}

	return InfoTextString;
}
