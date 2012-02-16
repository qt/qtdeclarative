function nameFromPath(path){
    var ret = path.split('/');
    return ret[ret.length-1].split('.')[0];
}
function iconFromPath(path){
    var ret = path.split('/');
    return "../images/launcherIcons/" + ret[ret.length-1].split('.')[0] + ".png";
}
