function [b, s, d, q, iq, id, is, ib] = dct_img( image, xblock, yblock, qtable, prevDC )
    if (nargin < 5)
        prevDC = 0;
    end
    
    xidx = xblock * 8 + 1;
    yidx = yblock * 8 + 1;
    b = image(yidx:yidx+7, xidx:xidx+7);
    s = b - 128;
    d = dct2(s);
    q = round(d ./ qtable);
    tmp = q;
    tmp(1,1) = tmp(1,1) - prevDC;
    iq = tmp .* qtable;
    id = idct2(iq);
    is = uint8(id);
    ib = is + 128;
end