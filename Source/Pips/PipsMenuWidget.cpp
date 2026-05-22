#include "PipsMenuWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "PipsGameInstance.h"

void UPipsMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (EasyButton)   EasyButton->OnClicked.AddDynamic(this,   &UPipsMenuWidget::OnEasyClicked);
    if (MediumButton) MediumButton->OnClicked.AddDynamic(this, &UPipsMenuWidget::OnMediumClicked);
    if (HardButton)   HardButton->OnClicked.AddDynamic(this,   &UPipsMenuWidget::OnHardClicked);
    if (PlayButton)   PlayButton->OnClicked.AddDynamic(this,   &UPipsMenuWidget::OnPlayClicked);

    // Default date = today.
    if (DateTextBox)
    {
        const FString Today = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
        DateTextBox->SetText(FText::FromString(Today));
    }

    if (StatusText)
    {
        StatusText->SetText(FText::GetEmpty());
    }

    UpdateDifficultyVisuals();
}

void UPipsMenuWidget::OnEasyClicked()   { SelectedDifficulty = EPipsDifficulty::Easy;   UpdateDifficultyVisuals(); }
void UPipsMenuWidget::OnMediumClicked() { SelectedDifficulty = EPipsDifficulty::Medium; UpdateDifficultyVisuals(); }
void UPipsMenuWidget::OnHardClicked()   { SelectedDifficulty = EPipsDifficulty::Hard;   UpdateDifficultyVisuals(); }

void UPipsMenuWidget::UpdateDifficultyVisuals()
{
    // Simple feedback — Blueprint can override if desired.
    auto Tint = [](UButton* B, bool bSelected)
    {
        if (!B) return;
        FLinearColor C = bSelected ? FLinearColor(0.3f, 0.6f, 1.0f) : FLinearColor::White;
        B->SetBackgroundColor(C);
    };
    Tint(EasyButton,   SelectedDifficulty == EPipsDifficulty::Easy);
    Tint(MediumButton, SelectedDifficulty == EPipsDifficulty::Medium);
    Tint(HardButton,   SelectedDifficulty == EPipsDifficulty::Hard);
}

void UPipsMenuWidget::OnPlayClicked()
{
    UPipsGameInstance* GI = GetGameInstance<UPipsGameInstance>();
    if (!GI)
    {
        if (StatusText) StatusText->SetText(FText::FromString(TEXT("Error: no GameInstance")));
        return;
    }

    const FString Date = DateTextBox
        ? DateTextBox->GetText().ToString()
        : FDateTime::Now().ToString(TEXT("%Y-%m-%d"));

    // Basic validation — must be YYYY-MM-DD.
    if (Date.Len() != 10 || Date[4] != '-' || Date[7] != '-')
    {
        if (StatusText) StatusText->SetText(FText::FromString(TEXT("Date must be YYYY-MM-DD")));
        return;
    }

    GI->SelectedDate = Date;
    GI->SelectedDifficulty = SelectedDifficulty;

    UGameplayStatics::OpenLevel(this, GameplayMapName);
}