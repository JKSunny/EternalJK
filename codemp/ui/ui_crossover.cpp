// ui_crossover.c -- Crossover API module for UI
// Copyright (c) 2013 Jedi Knight Galaxies

#include "ui_local.h"
#include "jkg_partymanager.h"
#include "jkg_inventory.h"

cgCrossoverExports_t *cgImports;
uiCrossoverExports_t ui;

/*
 *	The notify code on the UI for the shop menu
 */
void JKG_ForceItemMenuUpdates() {
	menuDef_t* focusedMenu = Menu_GetFocused();
	if (focusedMenu == nullptr) {
		return; // No focused menu?
	}
	if (!Q_stricmp(focusedMenu->window.name, "jkg_inventory")) {
		JKG_ConstructInventoryList();
	}
	else if (!Q_stricmp(focusedMenu->window.name, "jkg_shop")) {
		JKG_ShopInventorySortChanged();
	}
}

static qboolean coTrapEscape = qfalse;
void CO_SetEscapeTrapped( qboolean trapped )
{
	coTrapEscape = trapped;
}

uiCrossoverExports_t *UI_InitializeCrossoverAPI( cgCrossoverExports_t *cg )
{
	cgImports = cg;

	ui.HandleServerCommand = UI_RunSvCommand;
	ui.InventoryNotify = JKG_Inventory_UpdateNotify;
	ui.PartyMngtNotify = JKG_PartyMngt_UpdateNotify;
	ui.SetEscapeTrap = CO_SetEscapeTrapped;
	ui.ShopNotify = JKG_ShopNotify;
	ui.ItemsUpdated = JKG_ForceItemMenuUpdates;
	ui.InventoryPriceCheckResult = JKG_Shop_PriceCheckComplete;

	return &ui;
}
