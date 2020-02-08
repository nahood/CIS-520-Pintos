--------------------------------------------------------------------------------

threads/Rubric.alarm

Added a line containing "alarm-mega".

--------------------------------------------------------------------------------

threads/Make.tests

In the "Test names" section, added "alarm-mega" after "alarm-multiple".

--------------------------------------------------------------------------------

threads/tests.c

Added {"alarm-mega", test_alarm_mega} in the tests struct after the
"alarm-multiple entry.

--------------------------------------------------------------------------------

threads/alarm-wait.c

Added a test function called "test_alarm_mega" where it calls test_sleep(5, 70).

--------------------------------------------------------------------------------

threads/tests.h

Added a line "extern test_func test_alarm_mega;".

--------------------------------------------------------------------------------

threads/alarm-mega.ck

Created an "alarm-mega.ck" following the format of the "alarm-multiple.ck" file
and calling check_alarm(70) instead of check_alarm(7)
