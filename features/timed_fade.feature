Feature: Timed Fade
  In order to make life pleasant
  As a developer of spark device apps
  I want to fade light brightness over a given length of time

  Scenario: Fade from off to full over 10 seconds
    Given the light is off
    When I send the fade command with brightness 255 and duration 1000
    Then in 2.5 seconds the brightess is about 64
    And in 5 seconds the brightness is about 128
    And in 10 seconds the brightness is about 255
