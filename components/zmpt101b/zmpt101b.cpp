#include "zmpt101b.h"

#include "esphome/core/log.h"
#include <cmath>

namespace esphome
{
  namespace zmpt101b
  {
    static const char *TAG = "zmpt101b";

    void ZMPT101BSensor::setup()
    {
      this->offsetV = this->reference_ / 2.0;
      ESP_LOGW(TAG, "starting offset: %f", this->offsetV);
    }

    void ZMPT101BSensor::dump_config()
    {
      LOG_SENSOR("", "ZMPT101B Sensor", this);
      ESP_LOGCONFIG(TAG, "  Sample Duration: %d, Calibration: %f", this->number_of_samples_ * this->frequency_, this->calibration_);
      LOG_UPDATE_INTERVAL(this);
    }

    void ZMPT101BSensor::update()
    {
      this->min = 1000;
      this->max = -1000;
      this->filteredMax=-1000;
      this->filteredMin=1000;
      // Set timeout for ending sampling phase
      this->set_timeout("read", this->number_of_samples_ * this->frequency_, [this](){

      this->is_sampling_ = false;
      ESP_LOGW(TAG, "reading...");
    
        if (this->num_samples_ == 0) {
          // Shouldn't happen, but let's not crash if it does.]
          ESP_LOGW(TAG, "WARNING: got 0 samples, returning NAN");
          this->publish_state(NAN);
          return;
        }

        ESP_LOGW(TAG, "min/max: %.2f/%.2f", this->min, this->max);
        ESP_LOGW(TAG, "filtered min/max: %.2f/%.2f", this->filteredMin, this->filteredMax);
    
        float sampled_voltage = this->sample_sum_ / this->num_samples_;
        ESP_LOGW(TAG, "publish_state: %.2f", sampled_voltage);
        this->publish_state(sampled_voltage);
        });

      ESP_LOGW(TAG, "returning mean of %d samples", this->num_samples_);
      ESP_LOGW(TAG, "offset: %.2f", this->offsetV);

      // Set sampling values
      this->is_sampling_ = true;
      this->num_samples_ = 0;
      this->sample_sum_ = 0.0f;
    }

    void ZMPT101BSensor::loop()
    {
      this->calcV(20, this->frequency_);
      float voltage = this->Vrms; // extract Vrms into Variable
      if (std::isnan(voltage))
        return;
      this->sample_sum_ += voltage;
      this->num_samples_++;
    }

    //--------------------------------------------------------------------------------------
    // emon_calc procedure
    // Calculates realPower,apparentPower,powerFactor,Vrms,Irms,kWh increment
    // From a sample window of the mains AC voltage and current.
    // The Sample window length is defined by the number of half wavelengths or crossings we choose to measure.
    //--------------------------------------------------------------------------------------
    void ZMPT101BSensor::calcV(unsigned int crossings, unsigned int timeout)
    {
      unsigned int crossCount = 0;      // Used to measure number of times threshold is crossed.
      unsigned int numberOfSamples = 0; // This is now incremented

      //-------------------------------------------------------------------------------------------------------------------------
      // 1) Waits for the waveform to be close to 'zero' (mid-scale adc) part in sin curve.
      //-------------------------------------------------------------------------------------------------------------------------
      uint32_t start = millis(); // millis()-start makes sure it doesnt get stuck in the loop if there is an error.

      while (1) // the while loop...
      {
        startV = this->source_->sample(); //using the voltage waveform           
        if ((startV < (this->reference_ * 0.55)) && (startV > (this->reference_ * 0.45)))
          break; // check its within range
        if ((millis() - start) > timeout)
        {
          ESP_LOGW(TAG, "crossing 0 timeout, last voltage: %f", startV);
          // testing exit early
          this->Vrms = 0;
          return;
          //break;
        }
      }


      //-------------------------------------------------------------------------------------------------------------------------
      // 2) Main measurement loop
      //-------------------------------------------------------------------------------------------------------------------------
      start = millis();

      while ((crossCount < crossings) && ((millis() - start) < timeout))
      {
        numberOfSamples++;         // Count number of times looped.
        lastFilteredV = filteredV; // Used for delay/phase compensation

        //-----------------------------------------------------------------------------
        // A) Read in raw voltage and current samples
        //-----------------------------------------------------------------------------
        sampleV = this->source_->sample(); // Read in raw voltage signal
        // ESP_LOGD(TAG, "sample: %.4f", value);

        if (sampleV > this->max) this->max = sampleV;
        if (sampleV < this->min) this->min = sampleV;

        //-----------------------------------------------------------------------------
        // B) Apply digital low pass filters to extract the 2.5 V or 1.65 V dc offset,
        //     then subtract this - signal is now centred on 0 counts.
        //-----------------------------------------------------------------------------
        offsetV = offsetV + ((sampleV - offsetV) / this->reference_); // TODO not sure if V_REF is right here, was ADC_COUNTS
        filteredV = sampleV - offsetV;

        if (filteredV > this->filteredMax) this->filteredMax = filteredV;
        if (filteredV < this->filteredMin) this->filteredMin = filteredV;

        //-----------------------------------------------------------------------------
        // C) Root-mean-square method voltage
        //-----------------------------------------------------------------------------
        sqV = filteredV * filteredV; // 1) square voltage values
        sumV += sqV;                 // 2) sum

        //-----------------------------------------------------------------------------
        // G) Find the number of times the voltage has crossed the initial voltage
        //    - every 2 crosses we will have sampled 1 wavelength
        //    - so this method allows us to sample an integer number of half wavelengths which increases accuracy
        //-----------------------------------------------------------------------------
        lastVCross = checkVCross;
        if (sampleV > startV)
          checkVCross = true;
        else
          checkVCross = false;
        if (numberOfSamples == 1)
          lastVCross = checkVCross;

        if (lastVCross != checkVCross)
          crossCount++;
      }

      this->Vrms = sqrt(sumV / numberOfSamples) * this->calibration_;

      // Reset accumulators
      this->sumV = 0;
      //--------------------------------------------------------------------------------------
    }

  } // namespace zmpt101b
} // namespace esphome
