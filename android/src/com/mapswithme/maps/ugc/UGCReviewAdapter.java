package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.Adapter;
import android.text.format.DateFormat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class UGCReviewAdapter extends Adapter<UGCReviewAdapter.ViewHolder>
{
  private static final int MAX_COUNT = 3;

  @NonNull
  private ArrayList<UGC.Review> mItems = new ArrayList<>();

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext())
                                        .inflate(R.layout.item_ugc_comment, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position), position > 0);
  }

  @Override
  public int getItemCount()
  {
    return Math.min(mItems.size(), MAX_COUNT);
  }

  public void setItems(@NonNull List<UGC.Review> items)
  {
    this.mItems.clear();
    this.mItems.addAll(items);
    notifyDataSetChanged();
  }

  static class ViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    final View mDivider;
    @NonNull
    final TextView mAuthor;
    @NonNull
    final TextView mCommentDate;
    @NonNull
    final TextView mReview;

    public ViewHolder(View itemView)
    {
      super(itemView);
      mDivider = itemView.findViewById(R.id.v__divider);
      mAuthor = (TextView) itemView.findViewById(R.id.tv__user_name);
      mCommentDate = (TextView) itemView.findViewById(R.id.tv__comment_date);
      mReview = (TextView) itemView.findViewById(R.id.tv__review);
    }

    public void bind(UGC.Review review, boolean isShowDivider)
    {
      UiUtils.showIf(isShowDivider, mDivider);
      mAuthor.setText(review.getAuthor());
      mCommentDate.setText(review.getDaysAgo() + " days ago");
      mReview.setText(review.getText());
    }
  }
}
