var UserCache = function() {
  this._users = [];
}


UserCache.prototype.getById = function(id){
  for (var i=0; i < this._users.length; i++){
    var user = this._users[i];
    if (user.twitterId == id) {
      return user;
    }
  }
}
UserCache.prototype.getByName = function(name){
  for (var i=0; i < this._users.length; i++){
    var user = this._users[i];
    if (user.name == name)
        return user;
  }
}

UserCache.prototype.add = function(user){
  this._users[this._users.length] = user;
}


var cache = new UserCache;

function getById(id) {
    return cache.getById(id);
}

function getByName(name) {
    return cache.getByName(name);
}

function createTwitterUser(canvas) {
    var user = Qt.createQmlObject("import QtQuick 2.0; TwitterUser{}", canvas);
    user.canvas = canvas;
    cache.add(user);
    return user;
}
