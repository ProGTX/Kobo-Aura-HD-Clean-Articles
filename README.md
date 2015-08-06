# Kobo-Aura-HD-Clean-Articles
Meant to clean leftover Pocket articles on a Kobo Aura HD. It seems to work OK, but no guarantees.

Requires some extra work:
  1. Copy folders .kobo and .kobo-image from device memory to computer - e.g. in folder AuraDir
  2. Export AuraDir/.kobo/KoboReader.sqlite to AuraDir/sql/kobo.sql
    - Important to have a compact SQL format, as the program features very simple parsing
    - SqliteBrowser was used in testing
  3. Run program inside AuraDir
  4. Convert AuraDir/sql/kobo_new.sql to AuraDir/sql/KoboReader.sqlite
  5. Replace AuraDir/.kobo/KoboReader.sqlite with AuraDir/sql/KoboReader.sqlite
  6. Delete folders .kobo and .kobo-image on device
    - May be a good idea to have a backup
  7. Copy folders .kobo and .kobo-image from AuraDir to device

Requires a C++11 compiler.
