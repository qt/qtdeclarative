import QtQuick 2.14

Rectangle
{
  width: 100
  height: 100

  WorkerScript
  {
    source: "doRequest.mjs"
    Component.onCompleted:
    {
      sendMessage({"url": "https://example.com"});
    }
  }
}
