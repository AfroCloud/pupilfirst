module Startups
  # Returns fee applicable to a startup, taking into account applied coupon, and amounts already paid.
  class FeeAndCouponDataService
    # TODO: The base fee should probably be read from the startup entry.
    BASE_FEE = 100_000

    def initialize(startup)
      @startup = startup
    end

    def props
      {
        fee: fee_props,
        coupon: coupon_props
      }
    end

    private

    def fee_props
      total_fee = @startup.billing_founders_count * BASE_FEE

      # Discounted fee, for all founders in team.
      payable_fee = coupon.blank? ? total_fee : (total_fee * (coupon.discount_percentage / 100)).to_i

      paid_fee = @startup.payments.paid.sum(:amount)
      remaining_payable = payable_fee - paid_fee

      # Undiscounted EMI figure, for display.
      emi_undiscounted = (total_fee / 6).round
      calculated_emi = (payable_fee / 6).round

      # Actual EMI is the lower of calculated EMI, or remaining payable amount.
      emi = calculated_emi <= remaining_payable ? calculated_emi : remaining_payable

      {
        payableFee: payable_fee,
        emiUndiscounted: emi_undiscounted,
        emi: emi
      }
    end

    def coupon_props
      return if coupon.blank?

      {
        code: coupon.code,
        discount: coupon.discount_percentage,
        instructions: coupon.instructions
      }
    end

    def coupon
      @coupon ||= @startup.applied_coupon
    end
  end
end