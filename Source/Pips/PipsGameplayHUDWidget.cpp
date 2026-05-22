// .cpp
#include "PipsGameplayHUDWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UPipsGameplayHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UPipsGameplayHUDWidget::OnBackClicked);
}

void UPipsGameplayHUDWidget::OnBackClicked()
{
	UGameplayStatics::OpenLevel(this, MenuMapName);
}