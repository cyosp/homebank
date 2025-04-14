## Process this file with automake to produce Makefile.in

NULL =

public_icons_themes = \
	Default	\
	$(NULL)

public_icons = \
	$(NULL)

private_icons = \
	Default_actions_scalable_hb-document-new.svg \
	Default_actions_scalable_hb-document-open.svg \
	Default_actions_scalable_hb-document-print.svg \
	Default_actions_scalable_hb-document-save.svg \
	Default_actions_scalable_hb-account.svg \
	Default_actions_scalable_hb-archive.svg \
	Default_actions_scalable_hb-assign.svg \
	Default_actions_scalable_hb-budget.svg \
	Default_actions_scalable_hb-category.svg \
	Default_actions_scalable_hb-clear.svg \
	Default_actions_scalable_hb-donate.svg \
	Default_actions_scalable_hb-file-export.svg \
	Default_actions_scalable_hb-file-import.svg \
	Default_actions_scalable_hb-filter.svg \
	Default_actions_scalable_hb-go-down.svg \
	Default_actions_scalable_hb-go-up.svg \
	Default_actions_scalable_hb-help.svg \
	Default_actions_scalable_hb-legend.svg \
	Default_actions_scalable_hb-life-energy.svg \
	Default_actions_scalable_hb-ope-add.svg \
	Default_actions_scalable_hb-ope-cleared.svg \
	Default_actions_scalable_hb-ope-delete.svg \
	Default_actions_scalable_hb-ope-edit.svg \
	Default_actions_scalable_hb-ope-future.svg \
	Default_actions_scalable_hb-ope-herit.svg \
	Default_actions_scalable_hb-ope-multiedit.svg \
	Default_actions_scalable_hb-ope-reconciled.svg \
	Default_actions_scalable_hb-ope-show.svg \
	Default_actions_scalable_hb-payee.svg \
	Default_actions_scalable_hb-rate.svg \
	Default_actions_scalable_hb-rep-balance.svg \
	Default_actions_scalable_hb-rep-budget.svg \
	Default_actions_scalable_hb-rep-stats.svg \
	Default_actions_scalable_hb-rep-time.svg \
	Default_actions_scalable_hb-rep-vehicle.svg \
	Default_actions_scalable_hb-view-bar.svg \
	Default_actions_scalable_hb-view-column.svg \
	Default_actions_scalable_hb-view-donut.svg \
	Default_actions_scalable_hb-view-line.svg \
	Default_actions_scalable_hb-view-list.svg \
	Default_actions_scalable_hb-view-pie.svg \
	Default_actions_scalable_hb-view-progress.svg \
	Default_actions_scalable_hb-view-refresh.svg \
	Default_actions_scalable_hb-view-stack.svg \
	Default_actions_scalable_hb-view-stack100.svg \
	Default_status_scalable_hb-changes-allow.svg \
	Default_status_scalable_hb-changes-prevent.svg \
	Default_status_scalable_hb-gf-blue.svg \
	Default_status_scalable_hb-gf-green.svg \
	Default_status_scalable_hb-gf-none.svg \
	Default_status_scalable_hb-gf-orange.svg \
	Default_status_scalable_hb-gf-purple.svg \
	Default_status_scalable_hb-gf-red.svg \
	Default_status_scalable_hb-gf-yellow.svg \
	Default_status_scalable_hb-file-valid.svg \
	Default_status_scalable_hb-file-invalid.svg \
	Default_status_scalable_hb-item-added.svg \
	Default_status_scalable_hb-item-auto.svg \
	Default_status_scalable_hb-item-budget.svg \
	Default_status_scalable_hb-item-closed.svg \
	Default_status_scalable_hb-item-edited.svg \
	Default_status_scalable_hb-item-forced.svg \
	Default_status_scalable_hb-item-future.svg \
	Default_status_scalable_hb-item-import.svg \
	Default_status_scalable_hb-item-pending.svg \
	Default_status_scalable_hb-item-remind.svg \
	Default_status_scalable_hb-item-similar.svg \
	Default_status_scalable_hb-item-clear.svg \
	Default_status_scalable_hb-item-recon.svg \
	Default_status_scalable_hb-item-reconlock.svg \
	Default_status_scalable_hb-item-void.svg \
	Default_status_scalable_hb-pm-cash.svg \
	Default_status_scalable_hb-pm-ccard.svg \
	Default_status_scalable_hb-pm-check.svg \
	Default_status_scalable_hb-pm-dcard.svg \
	Default_status_scalable_hb-pm-deposit.svg \
	Default_status_scalable_hb-pm-directdebit.svg \
	Default_status_scalable_hb-pm-epayment.svg \
	Default_status_scalable_hb-pm-fifee.svg \
	Default_status_scalable_hb-pm-intransfer.svg \
	Default_status_scalable_hb-pm-none.svg \
	Default_status_scalable_hb-pm-mobphone.svg \
	Default_status_scalable_hb-pm-standingorder.svg \
	Default_status_scalable_hb-pm-transfer.svg \
	Default_status_scalable_hb-quicktips.svg \
	Default_status_scalable_prf-advanced.svg \
	Default_status_scalable_prf-backup.svg \
	Default_status_scalable_prf-euro.svg \
	Default_status_scalable_prf-folder.svg \
	Default_status_scalable_prf-forecast.svg \
	Default_status_scalable_prf-general.svg \
	Default_status_scalable_prf-import.svg \
	Default_status_scalable_prf-interface.svg \
	Default_status_scalable_prf-interface-theme.svg \
	Default_status_scalable_prf-interface-color.svg \
	Default_status_scalable_prf-locale.svg \
	Default_status_scalable_prf-report.svg \
	Default_status_scalable_prf-transaction.svg \
	Default_status_scalable_prf-transaction-dialog.svg \
	Default_status_scalable_prf-transaction-transfer.svg \
	Default_status_scalable_prf-transaction-payment.svg \
	$(NULL)


EXTRA_DIST = \
	$(public_icons)		\
	$(private_icons)	\
	$(noinst_DATA)		\
	$(NULL)

###############################################################################

gtk_update_icon_cache = gtk-update-icon-cache -f -t

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache in '$(DESTDIR)'"; \
		for theme in $(public_icons_themes); do \
			echo " > $(datadir)/icons/$$theme."; \
			$(gtk_update_icon_cache) $(datadir)/icons/$$theme; \
			$(gtk_update_icon_cache) $(DESTDIR)$(pkgdatadir)/icons/$$theme; \
		done; \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		for theme in $(public_icons_themes); do \
			echo "***   $(gtk_update_icon_cache) $(datadir)/icons/$$theme"; \
		done; \
	fi

install-icons:
	for theme in $(public_icons_themes); do \
		cp index.theme $(DESTDIR)$(pkgdatadir)/icons/$$theme/index.theme; \
	done; \
	for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

uninstall-icons:
	for theme in $(public_icons_themes); do \
		rm $(DESTDIR)$(pkgdatadir)/icons/$$theme/index.theme; \
	done; \
	-for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

install-data-local: install-icons update-icon-cache

uninstall-local: uninstall-icons update-icon-cache
