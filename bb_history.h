
#define history_size 6

class InputChannelProcessor {

  public:

  bool        Inited;
  State<bool> FilteredState;

  void Init (uint16_t lowPass, bool gestureLag, uint16_t intervalSmallMs, uint16_t  intervalBigMs) {
    m_lowPassMs = lowPass;
    m_gestureLag = gestureLag;
    m_intervalSmallMs = intervalSmallMs;
    m_intervalBigMs = intervalBigMs;
    Inited = true;
  }

  
  void Step( bool currentInput, uint32_t currentTime ) {
    uint16_t deltat = currentTime - m_lastTime;
    uint16_t deltat_trigger = currentTime - m_lastTriggerTime;

    m_currentDelta = deltat;

    //has no changes
    if (m_state == currentInput) {
      return;
    }

    m_lastTriggerTime = currentTime;

    //hi freq filter
    if (deltat_trigger < m_lowPassMs) {
      return;
    }

    //shift history
    for (int j = history_size-1; j > 0; j--) {
      m_timing[j] = m_timing[j-1];
    }
    //write history current step
    m_state = currentInput;
    m_timing[0] = deltat;
    m_lastTime = currentTime;
    m_handledOnce = false;
    m_selectedGesture = Gesture::Nope;
    m_selectedGestureTime = 0;
    m_currentDelta = 0;

    FilteredState.set(currentInput);
  }

//todo inverted input
Gesture GestureValidate(uint32_t currentTime) {

  const uint16_t small = m_intervalSmallMs;
  const uint16_t loong = m_intervalBigMs;

  if (m_handledOnce) {
    return Gesture::Nope;
  }

  uint16_t (&t)[history_size] = m_timing;

  Gesture newGesture = Gesture::Nope;

  if (!m_state && 
      t[0] < small && t[1] > small) {
      newGesture = Gesture::OneClick;
  }

  if (!m_state && 
      t[0] < small && t[1] < small && t[2] < small && t[3] > small) {
      newGesture = Gesture::DoubleClick;
  }

  if (!m_state && 
      t[0] < small && t[1] < small && t[2] < small && t[3] < small && t[4] < small && t[5] > small) {
      newGesture = Gesture::TripleClick;
  }

  if (!m_state && 
      t[0] > small && t[0] < loong && t[1] > small) {
      newGesture = Gesture::MediumClick;
  }

  if (m_state && 
      m_currentDelta > loong && t[0] < small && t[1] < small && t[2] > small) {
      newGesture = Gesture::DoubleHold;
  }

  if (m_state && 
      m_currentDelta > loong && t[0] > small) {
      newGesture = Gesture::Hold;
  }

  if (newGesture != Gesture::Nope && m_selectedGesture != newGesture) {
    m_selectedGesture = newGesture;
    m_selectedGestureTime = currentTime;
  }

  if (!m_gestureLag || 
        m_selectedGesture != Gesture::Nope && 
        currentTime - m_selectedGestureTime > small) {
    m_handledOnce = true;
    return m_selectedGesture;
  }

  return Gesture::Nope;
}

  private:

  bool      m_state;
  uint16_t  m_timing[history_size];
  uint32_t  m_lastTime;
  uint32_t  m_lastTriggerTime;
  uint32_t  m_currentDelta;
  bool      m_handledOnce;
  Gesture   m_selectedGesture;
  uint32_t  m_selectedGestureTime;
  uint16_t  m_lowPassMs;
  uint16_t  m_intervalSmallMs;
  uint16_t  m_intervalBigMs;
  bool      m_gestureLag;
};