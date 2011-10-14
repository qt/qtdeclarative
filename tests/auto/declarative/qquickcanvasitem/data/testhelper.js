function comparePixel(ctx,x,y,r,g,b,a, d)
{
   var c = ctx.getImageData(x,y,1,1).data;
   if (d === undefined)
      d = 0;
   r = Math.round(r);
   g = Math.round(g);
   b = Math.round(b);
   a = Math.round(a);

   if (Math.abs(c[0]-r)>d || Math.abs(c[1]-g)>d || Math.abs(c[2]-b)>d || Math.abs(c[3]-a)>d) {
      console.log('Pixel compare fail:\nactual  :[' + c[0]+','+c[1]+','+c[2]+','+c[3] + ']\nexpected:['+r+','+g+','+b+','+a+'] +/- '+d);
      return false;
   }
   return true;
}


