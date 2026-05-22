#include "PipsMenuGameMode.h"
#include "PipsMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

APipsMenuGameMode::APipsMenuGameMode()
{
	DefaultPawnClass = nullptr;
}

void APipsMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!MenuWidgetClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	UPipsMenuWidget* Menu = CreateWidget<UPipsMenuWidget>(PC, MenuWidgetClass);
	if (Menu)
	{
		Menu->AddToViewport();
		PC->SetInputMode(FInputModeUIOnly());
		PC->bShowMouseCursor = true;
	}
}