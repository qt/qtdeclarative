WorkerScript.onMessage = function(message)
{
  var req = new XMLHttpRequest();
  req.open("GET", message.url, true);
  req.send();
};
