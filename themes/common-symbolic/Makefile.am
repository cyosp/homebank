## Process this file with automake to produce Makefile.in

NULL =

public_icons_themes = \
	hicolor	\
	$(NULL)

public_icons = \
	$(NULL)

private_icons = \
	hicolor_actions_scalable_edit-split-symbolic.svg \
	hicolor_actions_scalable_eye-not-looking-symbolic.svg \
	hicolor_actions_scalable_list-collapse-all-symbolic.svg \
	hicolor_actions_scalable_list-duplicate-symbolic.svg \
	hicolor_actions_scalable_list-expand-all-symbolic.svg \
	hicolor_actions_scalable_list-merge-symbolic.svg \
	hicolor_actions_scalable_list-move-after-symbolic.svg \
	hicolor_actions_scalable_open-in-browser-symbolic.svg \
	hicolor_actions_scalable_open-menu-symbolic.svg \
	hicolor_actions_scalable_text-casesensitive-symbolic.svg \
	hicolor_actions_scalable_text-regularexpression-symbolic.svg \
	hicolor_actions_scalable_data-usage-symbolic.svg \
	hicolor_actions_scalable_edit-split-symbolic.svg \
	hicolor_actions_scalable_list-collapse-all-symbolic.svg \
	hicolor_actions_scalable_list-duplicate-symbolic.svg \
	hicolor_actions_scalable_list-expand-all-symbolic.svg \
	hicolor_actions_scalable_list-merge-symbolic.svg \
	hicolor_actions_scalable_list-move-after-symbolic.svg \
	hicolor_actions_scalable_text-casesensitive-symbolic.svg \
	hicolor_actions_scalable_text-regularexpression-symbolic.svg \
	hicolor_actions_scalable_data-usage-symbolic.svg \
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
