void setMuxport(int channelNumber)
{
  if (channelNumber > 3) return; //Error check

  switch (channelNumber)
  {
    case 0:
      digitalWrite(muxA, LOW);
      digitalWrite(muxB, LOW);
      break;
    case 1:
      digitalWrite(muxA, HIGH);
      digitalWrite(muxB, LOW);
      break;
    case 2:
      digitalWrite(muxA, LOW);
      digitalWrite(muxB, HIGH);
      break;
    case 3:
      digitalWrite(muxA, HIGH);
      digitalWrite(muxB, HIGH);
      break;
  }
}
