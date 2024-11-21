ZMPT101B Voltage Sensor
=======================

.. seo::
    :description: Instructions for setting up ZMPT101B sensors.
    :image: zmpt101b.jpg

The ZMPT101B Voltage sensor allows you to hook up a ZMPT101B to an analog
voltage sensor (like the :doc:`ADC sensor <adc>`) and convert the readings to measured single phase AC Voltage.

Please also see `this video guide <https://www.youtube.com/watch?v=Ieuqvo5qxRc>`__
as an introduction to the working principle of ZMPT101B sensors and how to hook them up to your device.

.. figure:: images/zmpt101b-ui.png
    :align: center
    :width: 80.0%

.. code-block:: yaml

    # Example configuration entry
    sensor:
        - platform: zmpt101b
		  pin: 34
		  name: "Measured Voltage"
		  frequency: 50hz
		  num_of_samples: 50
		  calibration: 86
		  phase_shift: 1.7


Configuration variables:
------------------------

- **name** (**Required**, string): The name of the sensor.
- **pin** (**Required**, string): The pin number of the esp module where sensor is connected.
- **frequency** (*Optional*, enum 50hz/60hz): Frequency of electrical signal.
  This acts as an interval to check take the sample from EmonLib. Defaults to ``50hz``.
- **num_of_samples** (*Optional*, integer): The number of samples to be averaged with.
  Higher values can increase accuracy. Note:- Update interval should be higher
  then number_of_samples*frequency, Ex:- 50*50=2500. So update interval should be
  higher then 2.5 secs.
- **calibration** (*Optional*, float): The calibration number to get the
  correct voltage reading from EmonLib. Defaults to ``84``.
- **phase_shift** (*Optional*, float): The phase_shift number to get the
  correct voltage reading from EmonLib. Defaults to ``1.7``.
- **update_interval** (*Optional*, :ref:`config-time`): The interval
  to check the sensor. Defaults to ``60s``.
- **id** (*Optional*, :ref:`config-id`): Manually specify the ID used for code generation.
- All other options from :ref:`Sensor <config-sensor>`.

Calibration
-----------

This sensor needs calibration to show correct values, for this you can use `this video guide <https://www.youtube.com/watch?v=Ieuqvo5qxRc>`__
The Calibration number you find using this guide will be used as a starting 
calibration number in yaml file. It will change a bit since calculations in
esphome are little different but not a lot.

See Also
--------

- `EMonLib <https://github.com/openenergymonitor/EmonLib>`__
- `ZMPT101B Guide <https://www.youtube.com/watch?v=Ieuqvo5qxRc>`__
- :apiref:`sensor/zmpt101b.h`
- :ghedit:`Edit`
