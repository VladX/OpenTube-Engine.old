include (FindGettext)

if (GETTEXT_FOUND)
 add_custom_target (translations ALL DEPENDS)
 file (GLOB poTplFile *.pot)
 file (GLOB poLangDirs *)
 foreach (poLangDir ${poLangDirs})
  get_filename_component (poLang "${poLangDir}" NAME)
  if (EXISTS "${poLangDir}/${poLang}.po")
   execute_process (COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --backup=none -s -U "${poLangDir}/${poLang}.po" "${poTplFile}")
   file (MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${poLang}/LC_MESSAGES")
   add_custom_command (TARGET translations COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -c -o \"${CMAKE_CURRENT_BINARY_DIR}/${poLang}/LC_MESSAGES/${APP_NAME}.mo\" \"${poLangDir}/${poLang}.po\")
  endif (EXISTS "${poLangDir}/${poLang}.po")
 endforeach (poLangDir)
endif (GETTEXT_FOUND)
