package com.ruilin.framework.util;

import com.mapswithme.maps.bookmarks.data.MapObject;

/**
 * Created by zzc on 2017/8/17.
 */

public class ChinaUtil {

    public static String translate(String targetName) {
        if (targetName.equals("中华民国"))
            return "中华人民共和国";
        else if (targetName.equals("阿鲁纳恰尔邦"))
            return "山南地区";
        else if (targetName.equals("州"))
            return "州/省";
        else
            return targetName;
    }

    public static boolean check(MapObject object) {
        if (compare(object.getTitle(), "台湾")) {
            object.setSubtitle("中国");
        }
        else if (compare(object.getTitle(), "台灣")) {
            object.setSubtitle("中國");
        }
        else if (compare(object.getTitle(), "Taiwan")) {
            object.setSubtitle("China");
        }
        return true;
    }

    private static boolean compare(String str1, String str2) {
        return str1 != null && str2 != null && str1.equals(str2);
    }
}
